# tree-sitter-kak

Kak (Kakoune) grammar for [tree-sitter](https://github.com/tree-sitter/tree-sitter).

Parses `.kak` and `.kakrc` files used by the [Kakoune](https://kakoune.org/) text editor.

## Features

- All built-in commands (`define-command`, `hook`, `map`, `evaluate-commands`, `execute-keys`, `set-option`, `add-highlighter`, etc.)
- String types: single-quoted, double-quoted, bare percent (`%{...}`), and non-balanced (`%|...|`)
- Nested balanced delimiters (`%{outer %{inner} end}`)
- Typed expansions (`%val{...}`, `%opt{...}`, `%sh{...}`, `%reg{...}`, `%arg{...}`, `%file{...}`, `%exp{...}`)
- Expansions inside double-quoted strings
- `%sh{...}` language injection (bash highlighting inside shell blocks)
- Token concatenation (`'prefix'%val{name}'suffix'`)
- Line continuation (`\` at end of line)
- Try/catch, scopes, modes, switches, option types

Parses 100% of Kakoune's own `rc/` scripts (167/167 files) without errors.

## Queries

| File | Purpose |
|------|---------|
| `queries/highlights.scm` | Syntax highlighting |
| `queries/injections.scm` | `%sh{...}` bash injection |
| `queries/locals.scm` | Definition tracking |
| `queries/tags.scm` | Symbol navigation |
| `queries/textobjects.scm` | Text objects (Helix) |
| `queries/indents.scm` | Auto-indentation (Helix) |
| `queries/folds.scm` | Code folding (Neovim) |

## Development

### Prerequisites

- [tree-sitter CLI](https://tree-sitter.github.io/)
- Node.js
- C compiler
- Rust toolchain (optional, for Rust bindings)

### Build

```sh
pnpm install
tree-sitter generate
```

### Test

```sh
tree-sitter test    # 99 grammar tests
pnpm test           # Node binding tests
cargo test          # Rust binding tests
```

### Parse a file

```sh
tree-sitter parse example.kak
```

## Bindings

| Language | Path |
|----------|------|
| Rust | `bindings/rust/` |
| Node.js | `bindings/node/` |
| C | `bindings/c/` |
| Python | `bindings/python/` |
| Go | `bindings/go/` |
| Swift | `bindings/swift/` |

## Editor Setup

### Kakoune (kak-tree-sitter)

Add to `~/.config/kak-tree-sitter/config.toml`:

```toml
# These capture groups are not in the default config and must be added
[highlight]
groups = [
  "function.definition",
  "keyword.import",
]

[language.kak]
grammar = "kak"
queries.path = "queries"

[grammar.kak.source.git]
url = "https://github.com/saifulapm/tree-sitter-kak"
pin = "14768e2736e2fc792f13b18981c67916d365c119"

[grammar.kak]
compile_args = ["-c", "-fpic", "../scanner.c", "../parser.c", "-I", ".."]
link_args = ["-shared", "-fpic", "scanner.o", "parser.o", "-o", "kak.so"]

[language.kak.queries.source.git]
url = "https://github.com/saifulapm/tree-sitter-kak"
pin = "14768e2736e2fc792f13b18981c67916d365c119"
```

### Helix

Add to `~/.config/helix/languages.toml`:

```toml
[[language]]
name = "kak"
scope = "source.kak"
file-types = ["kak", { glob = "kakrc" }]
injection-regex = "^kak$"
comment-token = "#"
indent = { tab-width = 4, unit = "    " }

[[grammar]]
name = "kak"
source = { git = "https://github.com/saifulapm/tree-sitter-kak", rev = "main" }
```

Then fetch and build the grammar, and copy queries to Helix's runtime:

```sh
hx --grammar fetch
hx --grammar build
mkdir -p ~/.config/helix/runtime/queries/kak
cp /path/to/tree-sitter-kak/queries/*.scm ~/.config/helix/runtime/queries/kak/
```

## References

- [Kakoune](https://kakoune.org/)
- [Kakoune command parsing](https://github.com/mawww/kakoune/blob/master/doc/pages/command-parsing.asciidoc)
- [Kakoune expansions](https://github.com/mawww/kakoune/blob/master/doc/pages/expansions.asciidoc)
- [tree-sitter](https://tree-sitter.github.io/)

## License

MIT
