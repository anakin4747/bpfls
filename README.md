# bpfls

A language server for [bpftrace](https://github.com/bpftrace/bpftrace)
scripts, built on bpftrace's own parser.

- **Diagnostics:** syntax errors as you type, every error in the file, with
  bpftrace's own messages.
- **Hover:** documentation from bpftrace's `language.md` and `stdlib.md` for
  probe types, config variables, language constructs and standard library
  helpers. The docs are compiled into the binary.

## Install

With Nix:

```sh
nix profile install git+https://github.com/anakin4747/bpfls  # bpfls on PATH
nix run git+https://github.com/anakin4747/bpfls             # or just run it
```

The flake also provides `packages.vscode-extension` and an overlay that adds
`bpfls` and `vscode-extensions.bpfls.bpfls`.

Without Nix, build with CMake (see Development) and put `build/bpfls` on
your `PATH`.

## Editors

bpfls speaks LSP over stdio and handles `.bt` files.

**Neovim** (0.11+):

```lua
vim.lsp.config('bpfls', {
    cmd = { 'bpfls' },
    filetypes = { 'bpftrace' },
    root_markers = { '.git' },
})
vim.lsp.enable('bpfls')
```

**VS Code / VSCodium:** install the extension in `vscode/`. It runs `bpfls`
from `PATH`; set `bpfls.serverPath` to use another binary.

## Development

Work inside the Nix dev shell (`nix develop`), which has the toolchain,
Neovim and VSCodium. `make nix`, `make demo` and `make vscode` enter it
themselves.

| Command | |
|---|---|
| `make tests` | build and run the unit tests (`make nix` from outside the shell) |
| `make e2e` | run the end-to-end tests against the real binary |
| `make demo` | open `tests/demo/hello.bt` in Neovim with bpfls |
| `make vscode` | open it in VSCodium with the extension |
| `nix flake check` | build the packages and run every test, as CI does |

Clone with `--recursive`: bpftrace, lsp-framework and cppspec are
submodules. bpfls compiles only bpftrace's parser, with a few changes kept
as patches in `patches/bpftrace/`.

## License

Apache-2.0
