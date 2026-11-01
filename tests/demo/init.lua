-- Neovim config for `make demo`: opens a bpftrace script with bpfls, built
-- at build/bpfls in the repository root, as its language server.

local here = vim.fs.dirname(debug.getinfo(1, 'S').source:sub(2))
local root = vim.fs.normalize(here .. '/../..')

vim.filetype.add({ extension = { bt = 'bpftrace' } })

vim.lsp.config('bpfls', {
    cmd = { root .. '/build/bpfls' },
    filetypes = { 'bpftrace' },
    root_markers = { '.git' },
})
vim.lsp.enable('bpfls')

-- Log the LSP traffic so it can be inspected with :LspLog.
vim.lsp.log.set_level('debug')
