// VS Code extension that runs bpfls as the language server for bpftrace
// scripts (.bt).

const fs = require('fs');
const path = require('path');
const vscode = require('vscode');
const { LanguageClient } = require('vscode-languageclient/node');

let client;

// The bpfls executable, in order of preference:
// 1. the bpfls.serverPath setting;
// 2. build/bpfls of the bpfls repository, when the extension runs from its
//    source tree (vscode/ -> ../build/bpfls), as `make vscode` does;
// 3. bpfls on PATH, as installed by a package.
function serverPath(context) {
    const configured = vscode.workspace
        .getConfiguration('bpfls')
        .get('serverPath');
    if (configured) {
        return configured;
    }
    const repoBuild = path.join(context.extensionPath, '..', 'build', 'bpfls');
    if (fs.existsSync(repoBuild)) {
        return repoBuild;
    }
    return 'bpfls';
}

async function activate(context) {
    const command = serverPath(context);
    client = new LanguageClient(
        'bpfls',
        'bpfls',
        { command },
        { documentSelector: [{ language: 'bpftrace' }] },
    );
    await client.start();
}

async function deactivate() {
    if (client) {
        await client.stop();
    }
}

module.exports = { activate, deactivate };
