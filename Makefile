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

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
