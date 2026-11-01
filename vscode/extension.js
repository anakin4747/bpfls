// VS Code extension that runs bpfls as the language server for bpftrace
// scripts (.bt).

const path = require('path');
const vscode = require('vscode');
const { LanguageClient } = require('vscode-languageclient/node');

let client;

// The bpfls executable: the bpfls.serverPath setting if set, otherwise the
// build/bpfls binary of the repository this extension lives in
// (vscode/ -> ../build/bpfls).
function serverPath(context) {
    const configured = vscode.workspace
        .getConfiguration('bpfls')
        .get('serverPath');
    if (configured) {
        return configured;
    }
    return path.join(context.extensionPath, '..', 'build', 'bpfls');
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
