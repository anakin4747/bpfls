// Runs inside VSCodium's extension host (--extensionTestsPath) to prove the
// extension works end to end: it opens tests/demo/hello.bt, waits for bpfls
// to publish diagnostics, and checks hover. Throwing fails the run, which
// makes VSCodium exit with a non-zero status.

const assert = require('assert');
const path = require('path');
const vscode = require('vscode');

const demo = path.resolve(__dirname, '..', '..', 'tests', 'demo',
                          'hello.bt');

// Polls `check` until it returns a truthy value or `ms` pass.
async function waitFor(what, check, ms = 20000) {
    const deadline = Date.now() + ms;
    for (;;) {
        const value = await check();
        if (value) {
            return value;
        }
        if (Date.now() > deadline) {
            throw new Error(`timed out waiting for ${what}`);
        }
        await new Promise((resolve) => setTimeout(resolve, 100));
    }
}

// The first line of the hover at the first occurrence of `needle` (plus
// `offset` characters) in `document`.
async function hoverHeading(document, needle, offset = 0) {
    const index = document.getText().indexOf(needle);
    assert.notStrictEqual(index, -1, `${needle} is not in the demo`);
    const position = document.positionAt(index + offset);
    const hovers = await vscode.commands.executeCommand(
        'vscode.executeHoverProvider', document.uri, position);
    const text = hovers
        .flatMap((hover) => hover.contents)
        .map((content) => (typeof content === 'string' ? content
                                                        : content.value))
        .join('\n');
    return text.split('\n')[0];
}

async function run() {
    const document = await vscode.workspace.openTextDocument(demo);
    await vscode.window.showTextDocument(document);
    assert.strictEqual(document.languageId, 'bpftrace');

    // The demo has five deliberately broken probes.
    const diagnostics = await waitFor('diagnostics', () => {
        const found = vscode.languages.getDiagnostics(document.uri);
        return found.length > 0 ? found : undefined;
    });
    assert.deepStrictEqual(
        diagnostics.map((d) => d.message),
        [
            'syntax: expected expression',
            "syntax: expected ';'",
            'syntax: invalid hex escape sequence',
            'syntax: trailing underscore in integer literal',
            'syntax: increment/decrement requires a variable or map',
        ]);
    assert.ok(diagnostics.every((d) => d.source === 'bpftrace'));

    const expected = [
        ['begin {', 0, '### begin/end'],
        ['tracepoint:raw_syscalls:sys_enter', 0, '### tracepoint'],
        ['max_strlen =', 0, '### max_strlen'],
        ['printf(', 0, '### printf'],
        ['sizeof(', 0, '*From language.md*'],
    ];
    for (const [needle, offset, heading] of expected) {
        assert.strictEqual(await hoverHeading(document, needle, offset),
                           heading, `hover on ${needle}`);
    }

    console.log(`bpfls extension works: ${diagnostics.length} diagnostics, ` +
                `${expected.length} hovers checked`);
}

module.exports = { run };
