BUILD_DIR ?= build

.PHONY: cqfd
cqfd:
	@./scripts/cqfd init > /dev/null
	@./scripts/cqfd run make tests

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
