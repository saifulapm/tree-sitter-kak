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
    EXPANSION_END,
    CONCAT,
};

#define MAX_DEPTH 64

typedef struct {
    int32_t delimiter_stack[MAX_DEPTH];
    uint8_t stack_size;
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

void *tree_sitter_kakscript_external_scanner_create() {
    Scanner *s = (Scanner *)ts_calloc(1, sizeof(Scanner));
    return s;
}

void tree_sitter_kakscript_external_scanner_destroy(void *payload) {
    ts_free(payload);
}

unsigned tree_sitter_kakscript_external_scanner_serialize(void *payload, char *buffer) {
    Scanner *s = (Scanner *)payload;
    unsigned size = 0;
    buffer[size++] = (char)s->stack_size;
    for (uint8_t i = 0; i < s->stack_size; i++) {
        buffer[size++] = (char)s->delimiter_stack[i];
    }
    return size;
}

void tree_sitter_kakscript_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
    Scanner *s = (Scanner *)payload;
    s->stack_size = 0;
    if (length == 0) return;
    unsigned pos = 0;
    s->stack_size = (uint8_t)buffer[pos++];
    for (uint8_t i = 0; i < s->stack_size && pos < length; i++) {
        s->delimiter_stack[i] = (int32_t)buffer[pos++];
    }
}

// --- Scanning ---

// Scan verbatim content inside a bare percent string.
// Handles nested balanced delimiters.
static bool scan_bare_content(Scanner *s, TSLexer *lexer) {
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
    int depth = 0;

    while (!lexer->eof(lexer)) {
        int32_t c = lexer->lookahead;

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
        lexer->mark_end(lexer);
        lexer->result_symbol = BARE_STRING_CONTENT;
        return true;
    }
    return false;
}

// Scan for %{ %[ %< %( or %| %/ etc.
static bool scan_percent_start(Scanner *s, TSLexer *lexer) {
    if (lexer->lookahead != '%') return false;

    advance_scanner(lexer); // consume '%'
    int32_t next = lexer->lookahead;

    // If alpha follows, this is a typed expansion (%sh, %val, etc.), not a bare percent string
    if ((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z')) {
        return false;
    }

    if (lexer->eof(lexer)) return false;

    if (is_balanced_open(next)) {
        int32_t close = closing_delimiter_for(next);
        if (s->stack_size < MAX_DEPTH) {
            s->delimiter_stack[s->stack_size++] = close;
        }
        advance_scanner(lexer); // consume opening delimiter
        lexer->mark_end(lexer);
        lexer->result_symbol = PERCENT_STRING_START;
        return true;
    }

    // Non-balanced: any non-alphanumeric, non-whitespace char
    if (next != ' ' && next != '\t' && next != '\n' && next != '\r') {
        if (s->stack_size < MAX_DEPTH) {
            s->delimiter_stack[s->stack_size++] = next;
        }
        advance_scanner(lexer); // consume delimiter
        lexer->mark_end(lexer);
        lexer->result_symbol = NONBALANCED_STRING_START;
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

bool tree_sitter_kakscript_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
    Scanner *s = (Scanner *)payload;

    // Guard against error recovery: if all symbols are valid, bail out
    // (unless we're legitimately inside a string)
    bool error_recovery = valid_symbols[PERCENT_STRING_START] &&
        valid_symbols[PERCENT_STRING_END] &&
        valid_symbols[BARE_STRING_CONTENT] &&
        valid_symbols[NONBALANCED_STRING_START] &&
        valid_symbols[NONBALANCED_STRING_END] &&
        valid_symbols[STRING_CONTENT_DOUBLE] &&
        valid_symbols[EXPANSION_PERCENT] &&
        valid_symbols[EXPANSION_END] &&
        valid_symbols[CONCAT];

    if (error_recovery) {
        return false;
    }

    // Inside a string, scan content first (no whitespace skipping — content is verbatim)
    if (valid_symbols[BARE_STRING_CONTENT] && s->stack_size > 0) {
        if (scan_bare_content(s, lexer)) return true;
    }

    // Check for closing delimiter (no whitespace skipping)
    if ((valid_symbols[PERCENT_STRING_END] || valid_symbols[NONBALANCED_STRING_END]) && s->stack_size > 0) {
        if (scan_percent_end(s, lexer)) return true;
    }

    // Skip whitespace before looking for percent string start
    if (valid_symbols[PERCENT_STRING_START] || valid_symbols[NONBALANCED_STRING_START]) {
        while (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\r') {
            skip_scanner(lexer);
        }
        if (scan_percent_start(s, lexer)) return true;
    }

    return false;
}
