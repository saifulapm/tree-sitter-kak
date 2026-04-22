#include "tree_sitter/parser.h"
#include "tree_sitter/alloc.h"

#include <string.h>

// Token order MUST match the externals array in grammar.js exactly.
enum TokenType {
    PERCENT_STRING_START,
    PERCENT_STRING_END,
    BARE_STRING_CONTENT,
    NONBALANCED_STRING_START,
    NONBALANCED_STRING_END,
    STRING_CONTENT_DOUBLE,
    EXPANSION_PERCENT,
    EXPANSION_DELIM_START,
    EXPANSION_END,
    CONCAT,
};

#define MAX_DEPTH 64

typedef struct {
    int32_t delimiter_stack[MAX_DEPTH];
    uint8_t stack_size;
    int16_t block_body_depth;  // persisted brace depth within block_body content
} Scanner;

static int32_t closing_delimiter_for(int32_t open) {
    switch (open) {
        case '{': return '}';
        case '[': return ']';
        case '<': return '>';
        case '(': return ')';
        default: return open;
    }
}

static bool is_balanced_open(int32_t c) {
    return c == '{' || c == '[' || c == '<' || c == '(';
}

static void advance_scanner(TSLexer *lexer) {
    lexer->advance(lexer, false);
}

static void skip_scanner(TSLexer *lexer) {
    lexer->advance(lexer, true);
}

// --- Lifecycle ---

void *tree_sitter_kak_external_scanner_create() {
    Scanner *s = (Scanner *)ts_calloc(1, sizeof(Scanner));
    return s;
}

void tree_sitter_kak_external_scanner_destroy(void *payload) {
    ts_free(payload);
}

unsigned tree_sitter_kak_external_scanner_serialize(void *payload, char *buffer) {
    Scanner *s = (Scanner *)payload;
    unsigned size = 0;

    if (1 + s->stack_size * sizeof(int32_t) > TREE_SITTER_SERIALIZATION_BUFFER_SIZE) {
        return 0;
    }

    buffer[size++] = (char)s->stack_size;
    for (uint8_t i = 0; i < s->stack_size; i++) {
        // Serialize full int32_t to avoid truncation of non-ASCII delimiters
        memcpy(&buffer[size], &s->delimiter_stack[i], sizeof(int32_t));
        size += sizeof(int32_t);
    }
    memcpy(&buffer[size], &s->block_body_depth, sizeof(int16_t));
    size += sizeof(int16_t);
    return size;
}

void tree_sitter_kak_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
    Scanner *s = (Scanner *)payload;
    s->stack_size = 0;
    s->block_body_depth = 0;
    if (length == 0) return;
    unsigned pos = 0;
    s->stack_size = (uint8_t)buffer[pos++];
    for (uint8_t i = 0; i < s->stack_size && pos + sizeof(int32_t) <= length; i++) {
        memcpy(&s->delimiter_stack[i], &buffer[pos], sizeof(int32_t));
        pos += sizeof(int32_t);
    }
    if (pos + sizeof(int16_t) <= length) {
        memcpy(&s->block_body_depth, &buffer[pos], sizeof(int16_t));
        pos += sizeof(int16_t);
    }
}

// --- Scanning ---

// Scan verbatim content inside a bare percent string.
// Handles nested balanced delimiters.
// When stop_at_expansion is true, stops before %alpha sequences (for block bodies).
static bool scan_bare_content(Scanner *s, TSLexer *lexer, bool stop_at_expansion) {
    if (s->stack_size == 0) return false;

    int32_t close_delim = s->delimiter_stack[s->stack_size - 1];
    int32_t open_delim = 0;

    switch (close_delim) {
        case '}': open_delim = '{'; break;
        case ']': open_delim = '['; break;
        case '>': open_delim = '<'; break;
        case ')': open_delim = '('; break;
    }

    bool has_content = false;
    int depth = stop_at_expansion ? s->block_body_depth : 0;
    bool in_single_quote = false;
    bool in_double_quote = false;
    // Line-comment state: '#' starts a comment when it appears at a token boundary
    // (start of content, after whitespace, or after newline). Comments run to EOL
    // and must suppress quote/expansion detection so e.g. `# %val{x}` isn't
    // mistaken for an expansion.
    bool in_line_comment = false;
    bool at_token_boundary = true;

    while (!lexer->eof(lexer)) {
        int32_t c = lexer->lookahead;

        if (in_line_comment) {
            if (c == '\n') {
                in_line_comment = false;
            }
            // Skip quote & expansion logic; fall through to delimiter/advance block.
        } else {
            // Enter line comment on '#' at a token boundary (only when block-body-like).
            if (stop_at_expansion && c == '#' && at_token_boundary &&
                !in_single_quote && !in_double_quote) {
                in_line_comment = true;
            } else {
                // Track quote state within content
                if (c == '\'' && !in_double_quote) {
                    in_single_quote = !in_single_quote;
                } else if (c == '"' && !in_single_quote) {
                    in_double_quote = !in_double_quote;
                }
            }
        }
        // Next char is at a token boundary iff current is whitespace/newline.
        at_token_boundary = (c == ' ' || c == '\t' || c == '\n' || c == '\r');

        // In block body mode, stop before %alpha (expansion start like %sh, %val).
        // Only when not inside quotes, not in a comment, and at brace depth 0.
        if (stop_at_expansion && !in_line_comment && !in_single_quote && !in_double_quote && c == '%') {
            // Peek ahead to check for a valid expansion type followed by a delimiter.
            // We need to verify this is really %sh{, %val{, etc. — not %s in printf.
            lexer->mark_end(lexer);
            advance_scanner(lexer); // past %
            int32_t c1 = lexer->lookahead;
            if ((c1 >= 'a' && c1 <= 'z') || (c1 >= 'A' && c1 <= 'Z')) {
                // Scan the type keyword (up to 4 chars)
                char type_buf[8] = {0};
                int type_len = 0;
                int32_t tc = c1;
                while (type_len < 7 && ((tc >= 'a' && tc <= 'z') || (tc >= 'A' && tc <= 'Z'))) {
                    type_buf[type_len++] = (char)tc;
                    advance_scanner(lexer);
                    tc = lexer->lookahead;
                }
                // Check if type is a known expansion AND followed by a delimiter
                bool known_type = (strcmp(type_buf, "sh") == 0 || strcmp(type_buf, "val") == 0 ||
                                   strcmp(type_buf, "opt") == 0 || strcmp(type_buf, "reg") == 0 ||
                                   strcmp(type_buf, "arg") == 0 || strcmp(type_buf, "file") == 0 ||
                                   strcmp(type_buf, "exp") == 0);
                // Must match scan_expansion_delim_start — reject %, whitespace, alphanumeric.
                // Otherwise we commit to an expansion that the delim scanner can't open.
                bool has_delim = (tc == '{' || tc == '[' || tc == '(' || tc == '<' ||
                                  (tc != ' ' && tc != '\t' && tc != '\n' && tc != '\r' &&
                                   tc != '%' &&
                                   !((tc >= 'a' && tc <= 'z') || (tc >= 'A' && tc <= 'Z') ||
                                     (tc >= '0' && tc <= '9'))));
                if (known_type && has_delim) {
                    // Confirmed expansion — save depth and return content before %
                    s->block_body_depth = (int16_t)depth;
                    if (has_content) {
                        lexer->result_symbol = BARE_STRING_CONTENT;
                        return true;
                    }
                    return false;
                }
            }
            // Not a valid expansion — consume % (and any peeked chars) as content
            has_content = true;
            lexer->mark_end(lexer);
            continue;
        }

        if (open_delim != 0) {
            // Balanced: track nested braces
            if (c == open_delim) {
                depth++;
                has_content = true;
                advance_scanner(lexer);
            } else if (c == close_delim) {
                if (depth > 0) {
                    depth--;
                    has_content = true;
                    advance_scanner(lexer);
                } else {
                    break; // end of string
                }
            } else {
                has_content = true;
                advance_scanner(lexer);
            }
        } else {
            // Non-balanced: close on same char
            if (c == close_delim) {
                break;
            } else {
                has_content = true;
                advance_scanner(lexer);
            }
        }
    }

    if (has_content) {
        if (stop_at_expansion) {
            s->block_body_depth = (int16_t)depth;
        }
        lexer->mark_end(lexer);
        lexer->result_symbol = BARE_STRING_CONTENT;
        return true;
    }
    return false;
}

// Scan text content inside double-quoted strings.
// Stops before closing " or before expansion start (%alpha).
// Consumes escaped quotes ("") and escaped percents (%%) as content.
// Returns false without advancing if the first char is " or %alpha (no content).
static bool scan_double_string_content(TSLexer *lexer, const bool *valid_symbols) {
    bool has_content = false;

    while (!lexer->eof(lexer)) {
        int32_t c = lexer->lookahead;

        if (c == '"') {
            // Peek: is next also "? (escaped quote)
            lexer->mark_end(lexer);
            advance_scanner(lexer);
            if (lexer->lookahead == '"') {
                // Escaped quote - consume both
                has_content = true;
                advance_scanner(lexer);
                lexer->mark_end(lexer);
                continue;
            }
            // Closing quote - stop, mark_end already set before the "
            break;
        }

        if (c == '%') {
            lexer->mark_end(lexer);
            advance_scanner(lexer);
            int32_t next = lexer->lookahead;

            if (next == '%') {
                // Escaped percent - consume both
                has_content = true;
                advance_scanner(lexer);
                lexer->mark_end(lexer);
                continue;
            }

            if ((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z')) {
                // Expansion start - stop before the %
                // mark_end was already called before the %
                if (!has_content && valid_symbols[EXPANSION_PERCENT]) {
                    // First char is % followed by alpha - emit EXPANSION_PERCENT instead
                    // We already consumed the %, just mark end after it
                    lexer->mark_end(lexer);
                    lexer->result_symbol = EXPANSION_PERCENT;
                    return true;
                }
                break;
            }

            // Lone % followed by non-alpha - it's content
            has_content = true;
            lexer->mark_end(lexer);
            continue;
        }

        has_content = true;
        advance_scanner(lexer);
        lexer->mark_end(lexer);
    }

    if (has_content) {
        lexer->result_symbol = STRING_CONTENT_DOUBLE;
        return true;
    }
    return false;
}

// Unified scan for all %-prefixed tokens:
// - EXPANSION_PERCENT: % followed by alpha (typed expansion start, emits just %)
// - PERCENT_STRING_START: %{ %[ %< %(
// - NONBALANCED_STRING_START: %| %/ etc.
static bool scan_percent_start(Scanner *s, TSLexer *lexer, const bool *valid_symbols, bool allow_nonbalanced) {
    if (lexer->lookahead != '%') return false;

    advance_scanner(lexer); // consume '%'
    int32_t next = lexer->lookahead;

    // If alpha follows, this is a typed expansion (%sh, %val, etc.)
    if ((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z')) {
        if (valid_symbols[EXPANSION_PERCENT]) {
            lexer->mark_end(lexer);
            lexer->result_symbol = EXPANSION_PERCENT;
            return true;
        }
        return false;
    }

    if (lexer->eof(lexer)) return false;

    if (is_balanced_open(next)) {
        if (valid_symbols[PERCENT_STRING_START]) {
            if (s->stack_size >= MAX_DEPTH) return false;
            s->delimiter_stack[s->stack_size++] = closing_delimiter_for(next);
            advance_scanner(lexer); // consume opening delimiter
            lexer->mark_end(lexer);
            lexer->result_symbol = PERCENT_STRING_START;
            return true;
        }
        return false;
    }

    // Non-balanced: any non-alphanumeric, non-whitespace char
    if (allow_nonbalanced && next != ' ' && next != '\t' && next != '\n' && next != '\r') {
        if (valid_symbols[NONBALANCED_STRING_START]) {
            if (s->stack_size >= MAX_DEPTH) return false;
            s->delimiter_stack[s->stack_size++] = next;
            advance_scanner(lexer); // consume delimiter
            lexer->mark_end(lexer);
            lexer->result_symbol = NONBALANCED_STRING_START;
            return true;
        }
        return false;
    }

    return false;
}

// Scan bare delimiter after expansion type keyword (e.g., the '{' in %val{...})
static bool scan_expansion_delim_start(Scanner *s, TSLexer *lexer) {
    int32_t c = lexer->lookahead;

    if (is_balanced_open(c)) {
        if (s->stack_size >= MAX_DEPTH) return false;
        s->delimiter_stack[s->stack_size++] = closing_delimiter_for(c);
        advance_scanner(lexer);
        lexer->mark_end(lexer);
        lexer->result_symbol = EXPANSION_DELIM_START;
        return true;
    }

    // Non-balanced: any non-alphanumeric, non-whitespace char
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '%' && !lexer->eof(lexer)
        && !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {
        if (s->stack_size >= MAX_DEPTH) return false;
        s->delimiter_stack[s->stack_size++] = c;
        advance_scanner(lexer);
        lexer->mark_end(lexer);
        lexer->result_symbol = EXPANSION_DELIM_START;
        return true;
    }

    return false;
}

// Scan closing delimiter
static bool scan_percent_end(Scanner *s, TSLexer *lexer) {
    if (s->stack_size == 0) return false;

    int32_t expected = s->delimiter_stack[s->stack_size - 1];
    if (lexer->lookahead == expected) {
        advance_scanner(lexer);
        lexer->mark_end(lexer);
        s->stack_size--;
        if (expected == '}' || expected == ']' || expected == '>' || expected == ')') {
            lexer->result_symbol = PERCENT_STRING_END;
        } else {
            lexer->result_symbol = NONBALANCED_STRING_END;
        }
        return true;
    }
    return false;
}

bool tree_sitter_kak_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
    Scanner *s = (Scanner *)payload;

    // Guard against error recovery: if all symbols are valid, bail out
    bool error_recovery = valid_symbols[PERCENT_STRING_START] &&
        valid_symbols[PERCENT_STRING_END] &&
        valid_symbols[BARE_STRING_CONTENT] &&
        valid_symbols[NONBALANCED_STRING_START] &&
        valid_symbols[NONBALANCED_STRING_END] &&
        valid_symbols[STRING_CONTENT_DOUBLE] &&
        valid_symbols[EXPANSION_PERCENT] &&
        valid_symbols[EXPANSION_DELIM_START] &&
        valid_symbols[EXPANSION_END] &&
        valid_symbols[CONCAT];

    if (error_recovery) {
        return false;
    }

    // CONCAT: zero-width token between adjacent tokens (no whitespace).
    // Emitted when the next char can start a new token (string, expansion, word)
    // and there's no whitespace between current position and that char.
    if (valid_symbols[CONCAT]) {
        int32_t c = lexer->lookahead;
        if (!(c == 0 || c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
              c == ';' || c == '#' || lexer->eof(lexer))) {
            lexer->result_symbol = CONCAT;
            return true;
        }
    }

    // Inside a double-quoted string, scan content segments or expansion start
    if (valid_symbols[STRING_CONTENT_DOUBLE]) {
        if (scan_double_string_content(lexer, valid_symbols)) return true;
    }

    // In block body mode (EXPANSION_PERCENT valid), check for expansion start BEFORE
    // scanning bare content, so %sh{} etc. are recognized as expansions.
    if (valid_symbols[EXPANSION_PERCENT] && valid_symbols[BARE_STRING_CONTENT] && s->stack_size > 0) {
        if (lexer->lookahead == '%') {
            if (scan_percent_start(s, lexer, valid_symbols, true)) return true;
        }
    }

    // Inside a string, scan content first (no whitespace skipping — content is verbatim)
    if (valid_symbols[BARE_STRING_CONTENT] && s->stack_size > 0) {
        if (scan_bare_content(s, lexer, valid_symbols[EXPANSION_PERCENT])) return true;
    }

    // Check for closing delimiter (no whitespace skipping)
    if ((valid_symbols[PERCENT_STRING_END] || valid_symbols[NONBALANCED_STRING_END]) && s->stack_size > 0) {
        if (scan_percent_end(s, lexer)) return true;
    }

    // Expansion delimiter start (bare delimiter after expansion type keyword, no whitespace skip)
    if (valid_symbols[EXPANSION_DELIM_START]) {
        if (scan_expansion_delim_start(s, lexer)) return true;
    }

    // Skip whitespace and line continuations before looking for percent string start or expansion percent
    if (valid_symbols[PERCENT_STRING_START] || valid_symbols[NONBALANCED_STRING_START] || valid_symbols[EXPANSION_PERCENT]) {
        bool had_whitespace = false;
        while (true) {
            if (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\r') {
                had_whitespace = true;
                skip_scanner(lexer);
            } else if (lexer->lookahead == '\\') {
                // Peek: is next char \n? (line continuation)
                skip_scanner(lexer);
                if (lexer->lookahead == '\n') {
                    had_whitespace = true;
                    skip_scanner(lexer); // consume \n — line continuation
                    continue;
                }
                // Not a continuation — \ already consumed as skip,
                // try scan_percent_start from current position
                break;
            } else {
                break;
            }
        }
        // Only allow non-balanced strings when preceded by whitespace.
        // Without whitespace, % is likely part of a key sequence (e.g. <a-%>).
        if (scan_percent_start(s, lexer, valid_symbols, had_whitespace)) return true;
    }

    return false;
}
