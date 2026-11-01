BUILD_DIR ?= build

SHELL_PREFIX :=
ifndef IN_NIX_SHELL
SHELL_PREFIX := nix develop --experimental-features 'nix-command flakes' --command
endif

.PHONY: nix
nix:
	@$(SHELL_PREFIX) make tests

.PHONY: build
build:
	@cmake -S . -B $(BUILD_DIR) -G Ninja
	@cmake --build $(BUILD_DIR)

.PHONY: test tests
test tests: build
	@cog check --from-latest-tag
	@ctest --test-dir $(BUILD_DIR) --output-on-failure --label-exclude e2e

# End-to-end tests run the real bpfls binary as a separate process. They are
# slower and use real I/O, so they run in CI on push rather than on every
# `make tests`.
.PHONY: e2e
e2e: build
	@ctest --test-dir $(BUILD_DIR) --output-on-failure --label-regex e2e \
		--no-tests=error

# Opens a bpftrace script in Neovim with bpfls as its language server. Both
# run from the dev shell; the build is pinned to build/, where
# tests/demo/init.lua expects the binary.
.PHONY: demo
demo:
ifndef IN_NIX_SHELL
	@$(SHELL_PREFIX) make demo
else
	@make build BUILD_DIR=build
	@nvim --clean -u tests/demo/init.lua tests/demo/hello.bt
endif

# The VS Code extension in vscode/, run in VSCodium from the dev
# shell. VSCodium gets a throwaway profile under build/vscode/ and all other
# extensions disabled, so personal settings neither leak in nor get changed.
# The build is pinned to build/, where the extension expects the binary.
VSCODE_EXTENSION := $(CURDIR)/vscode
CODIUM_FLAGS := \
	--user-data-dir $(CURDIR)/build/vscode/user \
	--extensions-dir $(CURDIR)/build/vscode/extensions \
	--disable-extensions \
	--disable-workspace-trust \
	--extensionDevelopmentPath=$(VSCODE_EXTENSION)
# The `codium` command hands off to a detached editor and returns at once,
# so tests run VSCodium's Electron binary directly to get its output and
# exit status. Only expanded inside the dev shell, where codium exists.
CODIUM_ELECTRON = $(shell dirname "$$(readlink -f "$$(command -v codium)")")/../lib/vscode/codium

.PHONY: vscode-extension
vscode-extension:
	@make build BUILD_DIR=build
	@npm --prefix $(VSCODE_EXTENSION) ci --no-audit --no-fund --silent

# Opens the demo script in VSCodium with the extension, until the window is
# closed.
.PHONY: vscode
vscode:
ifndef IN_NIX_SHELL
	@$(SHELL_PREFIX) make vscode
else
	@make vscode-extension
	@codium $(CODIUM_FLAGS) --new-window --wait $(CURDIR)/tests/demo/hello.bt
endif

# Runs the extension's smoke test in a headless VSCodium: it opens the demo
# script and checks bpfls's diagnostics and hover arrive. Fails if they
# don't.
.PHONY: vscode-test
vscode-test:
ifndef IN_NIX_SHELL
	@$(SHELL_PREFIX) make vscode-test
else
	@make vscode-extension
	@xvfb-run --auto-servernum $(CODIUM_ELECTRON) $(CODIUM_FLAGS) \
		--disable-gpu --extensionTestsPath=$(VSCODE_EXTENSION)/test/smoke.js
endif

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
