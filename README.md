# tree-sitter-kakscript

Kakscript (Kakoune) grammar for [tree-sitter](https://github.com/tree-sitter/tree-sitter).

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

## References

- [Kakoune](https://kakoune.org/)
- [Kakoune command parsing](https://github.com/mawww/kakoune/blob/master/doc/pages/command-parsing.asciidoc)
- [Kakoune expansions](https://github.com/mawww/kakoune/blob/master/doc/pages/expansions.asciidoc)
- [tree-sitter](https://tree-sitter.github.io/)

## License

MIT
