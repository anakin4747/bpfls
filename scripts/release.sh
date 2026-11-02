#!/usr/bin/env bash
#
# Cuts a release: works out the next version from the conventional commits
# since the last tag, then has cocogitto write CHANGELOG.md, bump the versions
# in flake.nix and vscode/package.json (see pre_bump_hooks in cog.toml),
# commit that as the release commit and tag it with an annotated tag carrying
# the changelog. Pushing the tag starts .github/workflows/release.yml, which
# creates the GitHub release; when it succeeds,
# .github/workflows/publish.yml publishes the VS Code extension.
#
# Usage: scripts/release.sh [--dry-run] [--no-push]

ME=$0

die() {
    printf "\033[31m${ME}: error: %s\033[0m\n" "$*" >&2
    exit 1
}

log() {
    printf "\033[32m${ME}: %s\033[0m\n" "$*"
}

usage() {
    printf 'usage: %s [--dry-run] [--no-push]\n' "$ME" >&2
    exit 1
}

: "${REMOTE:=origin}"
: "${BRANCH:=main}"

dry_run=0
push=1
for arg in "$@"; do
    case "$arg" in
        --dry-run) dry_run=1 ;;
        --no-push) push=0 ;;
        *) usage ;;
    esac
done

missing=""
for prog in git cog npm nix sed grep; do
    if ! command -v "$prog" > /dev/null; then
        missing="$missing $prog"
    fi
done
if [ -n "$missing" ]; then
    die "missing required programs:$missing"
fi

if ! cd "$(git rev-parse --show-toplevel)"; then
    die "changing to the repository root failed"
fi

branch=$(git branch --show-current)
if [ "$branch" != "$BRANCH" ]; then
    die "releases are cut from $BRANCH, not '$branch'"
fi

if [ -n "$(git status --porcelain)" ]; then
    die "the working tree has uncommitted or untracked files"
fi

log "fetching $REMOTE/$BRANCH and tags"
if ! git fetch --tags "$REMOTE" "$BRANCH"; then
    die "fetching $REMOTE/$BRANCH and tags failed"
fi
log "fetching $REMOTE/$BRANCH and tags succeeded"

if [ "$(git rev-parse HEAD)" != "$(git rev-parse "$REMOTE/$BRANCH")" ]; then
    die "$BRANCH is not in sync with $REMOTE/$BRANCH"
fi

log "checking commit messages since the latest tag"
if ! cog check --from-latest-tag; then
    die "checking commit messages since the latest tag failed"
fi
log "checking commit messages since the latest tag succeeded"

latest=$(git describe --tags --abbrev=0)
if [ -z "$latest" ]; then
    die "no previous release tag found"
fi

# First pass: ask cocogitto which version the commits call for.
log "determining the next version after $latest"
if ! next=$(cog bump --auto --dry-run); then
    die "determining the next version after $latest failed"
fi
case "$next" in
    v[0-9]*.[0-9]*.[0-9]*) ;;
    *) die "cog bump --auto --dry-run gave an unexpected version: '$next'" ;;
esac
if git rev-parse --quiet --verify "refs/tags/$next" > /dev/null; then
    die "tag $next already exists"
fi
log "determining the next version after $latest succeeded: $next"

# The changes since the last release, minus cocogitto's "## Unreleased"
# heading; the tag message gets its own heading.
log "generating the changelog for $latest..HEAD"
if ! changelog=$(cog changelog "$latest..HEAD"); then
    die "generating the changelog for $latest..HEAD failed"
fi
log "generating the changelog for $latest..HEAD succeeded"
changes=$(printf '%s\n' "$changelog" | sed '1{/^## /d}')
message=$(printf '%s\n\n%s\n' "$next" "$changes")

if [ "$dry_run" -eq 1 ]; then
    log "dry run: would release $next with this tag message:"
    printf '%s\n' "$message"
    exit 0
fi

# Second pass: write CHANGELOG.md, run the version hooks, create the release
# commit and the annotated tag.
log "bumping to $next"
if ! cog bump --auto --annotated "$message"; then
    die "bumping to $next failed"
fi
log "bumping to $next succeeded"

if [ "$(git describe --tags --exact-match HEAD)" != "$next" ]; then
    die "the release commit is not tagged $next"
fi
if [ "$(git cat-file -t "$next")" != "tag" ]; then
    die "$next is not an annotated tag"
fi

# The release commit changes the versions the Nix build reads; check it
# builds and passes before anything leaves this machine.
log "checking the $next release commit with nix flake check"
if ! nix flake check --print-build-logs; then
    die "checking the $next release commit with nix flake check failed;" \
        "undo with: git tag --delete $next && git reset --hard HEAD~1"
fi
log "checking the $next release commit with nix flake check succeeded"

if [ "$push" -eq 0 ]; then
    log "not pushing; run: git push --atomic $REMOTE $BRANCH $next"
    exit 0
fi

log "pushing $BRANCH and $next to $REMOTE"
if ! git push --atomic "$REMOTE" "$BRANCH" "$next"; then
    die "pushing $BRANCH and $next to $REMOTE failed"
fi
log "pushing $BRANCH and $next to $REMOTE succeeded"
