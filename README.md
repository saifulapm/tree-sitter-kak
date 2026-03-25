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

## Editor Setup

### Helix

Add to your `languages.toml` (usually `~/.config/helix/languages.toml`):

```toml
[[language]]
name = "kakscript"
scope = "source.kakscript"
file-types = ["kak", "kakrc"]
comment-tokens = "#"
indent = { tab-width = 4, unit = "    " }
grammar = "kakscript"

[[grammar]]
name = "kakscript"
source = { git = "https://github.com/saifulapm/tree-sitter-kakscript", rev = "main" }
```

Then fetch, build, and install queries:

```sh
hx --grammar fetch
hx --grammar build
```

Helix bundles queries in its own runtime directory. Copy them manually:

```sh
mkdir -p ~/.config/helix/runtime/queries/kakscript
git clone --depth 1 https://github.com/saifulapm/tree-sitter-kakscript /tmp/tree-sitter-kakscript
cp /tmp/tree-sitter-kakscript/queries/*.scm ~/.config/helix/runtime/queries/kakscript/
rm -rf /tmp/tree-sitter-kakscript
```

### Kakoune (kak-tree-sitter)

Add to your `kak-tree-sitter` config (`~/.config/kak-tree-sitter/config.toml`):

```toml
[language.kakscript]
remove_default_highlighter = true

[language.kakscript.grammar]
source.git = { url = "https://github.com/saifulapm/tree-sitter-kakscript", pin = "main" }
path = "src"

[language.kakscript.queries]
source.git = { url = "https://github.com/saifulapm/tree-sitter-kakscript", pin = "main" }
path = "queries"
```

Then sync:

```sh
ktsctl sync
```

### Neovim

Add to your Neovim tree-sitter config (e.g. using `nvim-treesitter`):

```lua
local parser_config = require("nvim-treesitter.parsers").get_parser_configs()

parser_config.kakscript = {
  install_info = {
    url = "https://github.com/saifulapm/tree-sitter-kakscript",
    files = { "src/parser.c", "src/scanner.c" },
    branch = "main",
  },
  filetype = "kak",
}

vim.filetype.add({
  extension = {
    kak = "kakscript",
    kakrc = "kakscript",
  },
})
```

Then install and copy queries:

```sh
:TSInstall kakscript
```

```sh
mkdir -p ~/.local/share/nvim/site/queries/kakscript
cp queries/*.scm ~/.local/share/nvim/site/queries/kakscript/
```

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
