# Changelog
All notable changes to this project will be documented in this file. See [conventional commits](https://www.conventionalcommits.org/) for commit guidelines.

- - -
## [v0.3.0](https://github.com/anakin4747/bpfls/compare/d627c279f719cb7fbce306ef834e0f39656cb102..v0.3.0) - 2026-11-02
#### Features
- (**diagnostics**) lint deprecated, removed and discouraged constructs - ([8ddbdf2](https://github.com/anakin4747/bpfls/commit/8ddbdf285e1f15436c37c412c5dcb9967205f143)) - Anakin Childerhose
- (**diagnostics**) report bpftrace's deprecation warnings - ([2045952](https://github.com/anakin4747/bpfls/commit/2045952cf9b481c38f3ac35a7604f1f7f68ca890)) - Anakin Childerhose
- (**formatting**) format documents with bpftrace's printer - ([def45ab](https://github.com/anakin4747/bpfls/commit/def45abd20959a53d6b042ce23a4960647610812)) - Anakin Childerhose
- (**server**) offer quick fixes as code actions - ([09dc095](https://github.com/anakin4747/bpfls/commit/09dc095e5e58d636369df82c77ebf5cebe504cdf)) - Anakin Childerhose
- (**server**) format whole documents - ([92ad02e](https://github.com/anakin4747/bpfls/commit/92ad02e0d1811206a30678ab3950fbb6b7a6d93e)) - Anakin Childerhose
- (**vscode**) bundle the bpfls binary in the extension - ([e45b2ce](https://github.com/anakin4747/bpfls/commit/e45b2ce0fed1b45f960280f0f55c58d807860c3c)) - Anakin Childerhose
#### Bug Fixes
- (**diagnostics**) check for deprecations in scripts with syntax errors - ([9b09a7b](https://github.com/anakin4747/bpfls/commit/9b09a7b987bf9d80f696a2cb02301ac9e6626fa6)) - Anakin Childerhose
#### Documentation
- document diagnostics and code actions - ([0ef8b13](https://github.com/anakin4747/bpfls/commit/0ef8b134a60ae3eac2d9ad34b6d8939eddf6d8c6)) - Anakin Childerhose
- document whole-document formatting - ([2453de5](https://github.com/anakin4747/bpfls/commit/2453de583d93cd2c34263b38d2d64937925c9a02)) - Anakin Childerhose
#### Build system
- run the e2e tests in the bpfls check instead of a second build - ([59f8b8f](https://github.com/anakin4747/bpfls/commit/59f8b8f0cb6d7f3d2ffe081e1eae944067217eef)) - Anakin Childerhose
#### CI
- publish the VS Code extension to the Marketplace - ([c5abad0](https://github.com/anakin4747/bpfls/commit/c5abad0969c6df98d2ea99ca4a7d5a0cd2d7b3e3)) - Anakin Childerhose
#### Refactoring
- (**nix**) inline package definitions into flake.nix - ([d627c27](https://github.com/anakin4747/bpfls/commit/d627c279f719cb7fbce306ef834e0f39656cb102)) - Anakin Childerhose
#### Chores
- (**build**) compile bpftrace's config and deprecation pass - ([38a307e](https://github.com/anakin4747/bpfls/commit/38a307e06a53b3ef8df33e47662814875cb53de5)) - Anakin Childerhose
- (**build**) optimise the flake build with -O2 - ([4b841ef](https://github.com/anakin4747/bpfls/commit/4b841ef9e43274469fcbfaddb65ad192e6cc97d5)) - Anakin Childerhose
- (**build**) precompile standard library headers - ([e833ba6](https://github.com/anakin4747/bpfls/commit/e833ba68f67972fa25bbbcf78e0388f6580fd0b0)) - Anakin Childerhose
- (**build**) build the flake package in release mode - ([6da9e62](https://github.com/anakin4747/bpfls/commit/6da9e628df086ca60f50c294d205ac56417e81cb)) - Anakin Childerhose
- (**build**) build tests in the flake only for checks - ([200d0a8](https://github.com/anakin4747/bpfls/commit/200d0a8afc17873caf723e2e4c3ee1c0fb7915ae)) - Anakin Childerhose
- (**build**) make tests opt-in with BPFLS_BUILD_TESTS - ([28d6ad3](https://github.com/anakin4747/bpfls/commit/28d6ad3e446cd334921734135ae93335e05c97b2)) - Anakin Childerhose
- add todo for hints to never use shorthand notation - ([a4a16ea](https://github.com/anakin4747/bpfls/commit/a4a16ea269f2e8ea40b4b4b3747483b5be5fc2d2)) - Anakin Childerhose
- note that macro bodies are not linted - ([6693c57](https://github.com/anakin4747/bpfls/commit/6693c57e578a00a51b1f262f29561c34e79a40fc)) - Anakin Childerhose
- use the git skill for any change to the codebase - ([0035f68](https://github.com/anakin4747/bpfls/commit/0035f6891539c41468eec2ad299891f471b2d426)) - Anakin Childerhose
- list deprecation diagnostics and code actions in TODO - ([045fa35](https://github.com/anakin4747/bpfls/commit/045fa35fb78c9284ed0fcc088277caceff09e025)) - Anakin Childerhose
- add TODO for config option diagnostics - ([2b73bce](https://github.com/anakin4747/bpfls/commit/2b73bce8d6d419991f7f68b505e231496c4fee1d)) - Anakin Childerhose
- remove finished TODOs - ([bcdd6ec](https://github.com/anakin4747/bpfls/commit/bcdd6ec73d2b15cf4752333d8e0de0bf806c60b4)) - Anakin Childerhose

- - -

## [v0.2.1](https://github.com/anakin4747/bpfls/compare/9c1e59efdab2e530a26d484138852ffef57b6fb7..v0.2.1) - 2026-11-01
#### CD
- check the release commit builds before pushing it - ([0b8a933](https://github.com/anakin4747/bpfls/commit/0b8a933812bb7b41fc05fdc04c8c66ffdc0404ab)) - Anakin Childerhose
#### Bug Fixes
- (**build**) fetch extension npm deps without a hash that releases break - ([9c1e59e](https://github.com/anakin4747/bpfls/commit/9c1e59efdab2e530a26d484138852ffef57b6fb7)) - Anakin Childerhose

- - -

## [v0.2.0](https://github.com/anakin4747/bpfls/compare/6bcb9716e83c885617c503d5fc6f4d1a30c74a1e..v0.2.0) - 2026-11-01
#### CD
- publish GitHub releases from annotated tags - ([ef9fe8a](https://github.com/anakin4747/bpfls/commit/ef9fe8aa2b41bbddf98e8580519565d592dae32e)) - Anakin Childerhose
- cut releases with make release using cocogitto - ([0428281](https://github.com/anakin4747/bpfls/commit/04282819c5705c330d7602e5f20859904c33d463)) - Anakin Childerhose
#### Features
- (**document**) find the innermost node at a position - ([61fecc6](https://github.com/anakin4747/bpfls/commit/61fecc6395b6f33c423a8226cc4fb14a73af94a4)) - Anakin Childerhose
- (**document**) declare node_at for looking up a node by position - ([fcf182e](https://github.com/anakin4747/bpfls/commit/fcf182e21a065948e79bdcd4c2a9c41ffa77ab48)) - Anakin Childerhose
- (**server**) hover docs for the standard library - ([1c8844a](https://github.com/anakin4747/bpfls/commit/1c8844a180004901b9ce591038dbe7a5454ebb1f)) - Anakin Childerhose
- (**server**) hover docs for probes, config and language constructs - ([857dc88](https://github.com/anakin4747/bpfls/commit/857dc88455806a46b9f2de014aab93f1487241eb)) - Anakin Childerhose
- (**server**) publish parser diagnostics on open and change - ([91a639a](https://github.com/anakin4747/bpfls/commit/91a639acbdde5cff8daf6703201a9f40576099a7)) - Anakin Childerhose
- (**server**) track open documents and hover begin/end probes - ([642bb65](https://github.com/anakin4747/bpfls/commit/642bb653f7ceb692e8dec7e907e87fe2d003870e)) - Anakin Childerhose
- (**server**) answer the LSP lifecycle: initialize, shutdown and exit - ([9039388](https://github.com/anakin4747/bpfls/commit/90393889f801bbd7cffb43d7ef2def6dc31da16d)) - Anakin Childerhose
- (**vscode**) find bpfls on PATH by default - ([18551a6](https://github.com/anakin4747/bpfls/commit/18551a6796bbc2d878b45d8d880239f4d202344f)) - Anakin Childerhose
- add a VS Code extension and make vscode to try it in VSCodium - ([05b83dc](https://github.com/anakin4747/bpfls/commit/05b83dc8fa2244c498694d1df4fca697bbaa1835)) - Anakin Childerhose
- embed bpftrace docs sections at build time - ([607273e](https://github.com/anakin4747/bpfls/commit/607273e1d07f885aa0237388e07e1599bd28d7ba)) - Anakin Childerhose
- add the bpfls executable serving LSP over stdio - ([a911916](https://github.com/anakin4747/bpfls/commit/a911916bff2f70b362f29b56a0158c9cac491668)) - Anakin Childerhose
- report every syntax error by patching bpftrace's parser to recover - ([6b3f093](https://github.com/anakin4747/bpfls/commit/6b3f093fbbddc1efec1fa7003c71e3dac8d4d807)) - Anakin Childerhose
- store a script in a Document parsed by bpftrace's parser - ([9430ca4](https://github.com/anakin4747/bpfls/commit/9430ca4dd2fa717bd2e24fd50f9f9fa6d90a24c1)) - Anakin Childerhose
#### Bug Fixes
- (**build**) compile all of bpftrace from one patched copy - ([96dd0b2](https://github.com/anakin4747/bpfls/commit/96dd0b25df0926b96cd916224b120a3564617324)) - Anakin Childerhose
- (**server**) treat BEGIN and END like begin and end in hover - ([b103056](https://github.com/anakin4747/bpfls/commit/b1030563732279b0fd3084b0df5bcbd44ea7a828)) - Anakin Childerhose
#### Documentation
- add a README - ([3964230](https://github.com/anakin4747/bpfls/commit/39642300ce9ab71180637e66c6d103f3867076df)) - Anakin Childerhose
#### Tests
- (**document**) specify node_at lookup by position - ([6f256b2](https://github.com/anakin4747/bpfls/commit/6f256b2fa94db125d8bfe21d7d0ec805db3f0236)) - Anakin Childerhose
- (**e2e**) start bpfls as a process and walk the LSP lifecycle - ([5f11bb1](https://github.com/anakin4747/bpfls/commit/5f11bb124d9cfe2b5895f18b956be813649fb398)) - Anakin Childerhose
- (**server**) specify hover docs for the standard library - ([c672197](https://github.com/anakin4747/bpfls/commit/c6721970d5ef384261572ee5b82606217bce3e99)) - Anakin Childerhose
- (**server**) specify hover docs for probes, config and constructs - ([34cb4ca](https://github.com/anakin4747/bpfls/commit/34cb4ca3c799c060f94979597e2ab47ac115bf61)) - Anakin Childerhose
- (**server**) specify publishing parser diagnostics - ([8533883](https://github.com/anakin4747/bpfls/commit/8533883f60a2f22a757a07c20bae2336c67633b6)) - Anakin Childerhose
- (**server**) specify hover docs for begin/end probes - ([2e90602](https://github.com/anakin4747/bpfls/commit/2e90602841e2b9af4b2dd7dba97e19adeb12263c)) - Anakin Childerhose
- (**server**) specify startup and shutdown over an in-memory stream - ([3126343](https://github.com/anakin4747/bpfls/commit/312634368fc25c378100a6abdae9882ea731f51a)) - Anakin Childerhose
- check lsp-framework's generated types deserialize - ([19e39c4](https://github.com/anakin4747/bpfls/commit/19e39c4d35344a5dfc039226826b40778b6398a6)) - Anakin Childerhose
#### Build system
- add a static bpfls flake output for release binaries - ([f1c2cb7](https://github.com/anakin4747/bpfls/commit/f1c2cb7c9e2b94f0004e68969ce0a1e639b7e51b)) - Anakin Childerhose
- package bpfls and the VS Code extension in the flake - ([10228c9](https://github.com/anakin4747/bpfls/commit/10228c9546eebcc5898b1e1635118d8032d2b638)) - Anakin Childerhose
- install the bpfls binary - ([6a054eb](https://github.com/anakin4747/bpfls/commit/6a054ebb9e665f914f45f8e3bc97f781548e19c8)) - Anakin Childerhose
- remove the headless VSCodium smoke test - ([3a0de4c](https://github.com/anakin4747/bpfls/commit/3a0de4cb6fbf6a314474558e1d9a5cf1e5cb96e3)) - Anakin Childerhose
- add node, vscodium and xvfb-run to the dev shell - ([75b6730](https://github.com/anakin4747/bpfls/commit/75b673016720c792eaf316030875a7643b9eca90)) - Anakin Childerhose
- show every standard library hover in the demo script - ([084d1ae](https://github.com/anakin4747/bpfls/commit/084d1ae93ded043deade8ffb3026f605a14b0578)) - Anakin Childerhose
- show every hover target in the demo script - ([a1b9208](https://github.com/anakin4747/bpfls/commit/a1b92080a940d965cb75c57edf6070e6be9c1450)) - Anakin Childerhose
- add deliberate syntax errors to the demo script - ([f547a5b](https://github.com/anakin4747/bpfls/commit/f547a5b07080684304897c7bef761aaa89fa76d5)) - Anakin Childerhose
- patch bpftrace to make Diagnostics::foreach public - ([b83a998](https://github.com/anakin4747/bpfls/commit/b83a998681767039822132325e58cb1ae82b0495)) - Anakin Childerhose
- use lowercase begin/end in the demo script - ([b516c43](https://github.com/anakin4747/bpfls/commit/b516c43bf7c1ce2f13663529571440abfe47aa3e)) - Anakin Childerhose
- add make demo to try bpfls in neovim - ([3e9bcea](https://github.com/anakin4747/bpfls/commit/3e9bcea7e5a6f329d72ae90e5e9565c6ea5445b0)) - Anakin Childerhose
- add neovim to the dev shell - ([06dfdcf](https://github.com/anakin4747/bpfls/commit/06dfdcf4c5a0618ff206a7a84010f6ddf8b00211)) - Anakin Childerhose
- vendor lsp-framework as a git submodule - ([bc8f5d6](https://github.com/anakin4747/bpfls/commit/bc8f5d686ae18d806876bf344ccf5606fa88d86f)) - Anakin Childerhose
- only call nix outside of nix shell - ([97946cc](https://github.com/anakin4747/bpfls/commit/97946ccc15e25b6d0ccdbb58d0e1b474d2e6c0eb)) - Anakin Childerhose
- apply bpftrace patches with git apply - ([a35b11f](https://github.com/anakin4747/bpfls/commit/a35b11f4ef937a48e9469c27eaf984f06424c4ab)) - Anakin Childerhose
- symlink compile_commands.json into the source root - ([0f9b896](https://github.com/anakin4747/bpfls/commit/0f9b896e38b224007875c37ad77e91c72611ee7b)) - Anakin Childerhose
- replace cqfd with a nix dev shell - ([22962ae](https://github.com/anakin4747/bpfls/commit/22962aef4b4e35d575cbb7445e9121323f4c67be)) - Anakin Childerhose
- stop building bpftrace - ([4eac797](https://github.com/anakin4747/bpfls/commit/4eac7979d781953a9f8b81f1dadef6f594db54ba)) - Anakin Childerhose
- vendor bpftrace and cppspec as git submodules - ([10c0340](https://github.com/anakin4747/bpfls/commit/10c0340fa048ddf2c1ec0ca64b029d636adf5277)) - Anakin Childerhose
- add bpftrace as a dependency - ([f49301a](https://github.com/anakin4747/bpfls/commit/f49301a411164931f17ad3f0da406cc764ac28de)) - Anakin Childerhose
- configure, build and run ctest in make tests - ([c88c435](https://github.com/anakin4747/bpfls/commit/c88c4359a97195f9d9904ba2c9f225467354a330)) - Anakin Childerhose
- add CMake project with cppspec hello world test - ([4b82d8e](https://github.com/anakin4747/bpfls/commit/4b82d8e077dbf09495ea9327646db38a1a040abe)) - Anakin Childerhose
- replace neovim tooling with C++ toolchain in cqfd image - ([28c6e1d](https://github.com/anakin4747/bpfls/commit/28c6e1dd7e66af1e37300c909025e4e9f5d1b382)) - Anakin Childerhose
- set .cqfdrc config file - ([6bcb971](https://github.com/anakin4747/bpfls/commit/6bcb9716e83c885617c503d5fc6f4d1a30c74a1e)) - Anakin Childerhose
#### CI
- run tests on branch pushes only - ([0f307a9](https://github.com/anakin4747/bpfls/commit/0f307a9026b1b535e3a58a328f4bca17a7732ac4)) - Anakin Childerhose
- run nix flake check on push - ([6f5d022](https://github.com/anakin4747/bpfls/commit/6f5d0221a7fcc022d067a764288231606cc1692b)) - Anakin Childerhose
- run end-to-end tests on push - ([7e757b7](https://github.com/anakin4747/bpfls/commit/7e757b780c18902c859c142c47b026e4db1ca6ed)) - Anakin Childerhose
#### Refactoring
- (**document**) take lsp::Position in node_at - ([2185d4d](https://github.com/anakin4747/bpfls/commit/2185d4da009a745759e3a9bafcd9108f06b1c2b4)) - Anakin Childerhose
#### Chores
- add TODOs - ([9d1343c](https://github.com/anakin4747/bpfls/commit/9d1343c474f2b93c9ba02f3acb8cebcd7a8da6d6)) - Anakin Childerhose
- ignore clangd cache folder - ([ae62fea](https://github.com/anakin4747/bpfls/commit/ae62fea86de891a50453a2bc0dac5c727e9bc7cb)) - Anakin Childerhose
#### Style
- indent with 4 spaces via clang-format - ([d85452d](https://github.com/anakin4747/bpfls/commit/d85452d00a0ed02ec5d4e9b57d6efa2bccad25ac)) - Anakin Childerhose

- - -

Changelog generated by [cocogitto](https://github.com/cocogitto/cocogitto).