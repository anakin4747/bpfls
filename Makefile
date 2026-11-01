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
	@ctest --test-dir $(BUILD_DIR) --output-on-failure

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
