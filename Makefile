
.PHONY: cqfd
cqfd:
	@./scripts/cqfd init > /dev/null
	@./scripts/cqfd run make tests

.PHONY: test tests
test tests:
	@cog check --from-latest-tag
