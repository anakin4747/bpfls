
Completion

---


rename

---

function signatures

---

semantic diagnostics not just parser diagnostics

Invalid config keys and values are reported (via bpftrace's Config).

---

# Deprecation and recommendation diagnostics

Known gap: the AST visitor skips `macro` bodies (bpftrace expands macros in
a later pass bpfls does not run), so nothing below is reported inside a
macro definition.

Planned bpfls diagnostics for deprecated, removed or discouraged bpftrace
constructs. Refs are relative to `bpftrace/`. "bpftrace emits" means bpftrace
already reports it, so bpfls may be able to run that pass instead of
reimplementing the check. "Code action" says whether bpfls can offer a quick
fix:

- easy: a single text replacement on a range bpfls already has from the AST
- moderate: the new text is built from AST parts, or several edits are needed
- none: no fix can be generated automatically, or it could change behaviour
- n/a: the hint is informational and there is no single fix to apply

## Deprecated syntax and config

### Syntax

- [x] `while` loops
  - Fix: range `for ($i : a..b)`
  - Severity: warning
  - Code action: none. The condition and update can be anything, so a
    rewrite could change what the loop does.
  - Refs: docs/language.md:606, :761; bpftrace emits:
    src/ast/passes/deprecated.cpp:88
- [x] `kfunc` / `kretfunc` probe types
  - Fix: `fentry` / `fexit`
  - Severity: hint
  - Code action: easy. Replace the probe type text.
  - Refs: docs/language.md:1373; silent alias: src/probe_types.h:85, :89
- [x] `delete(@m[key])`
  - Fix: `delete(@m, key)`
  - Severity: hint
  - Code action: moderate. Build `delete(@m, key)` from the map and key
    expressions; with more than one key, `@m[a, b]` becomes
    `delete(@m, (a, b))`.
  - Refs: docs/stdlib.md:297, :323; silently rewritten:
    src/ast/passes/map_sugar.cpp:281
- [x] `->` field access
  - Fix: `.`
  - Severity: hint
  - Code action: easy. Replace `->` with `.`, but the `->` token has to be
    found in the source text because the AST may not store its position.
  - Refs: docs/language.md:1943 (documented as legacy)

### Config keys

- [x] Deprecated, no effect: `symbol_source`, `max_type_res_iterations`,
      `unstable_addr`, `unstable_import`, `unstable_macro`,
      `unstable_map_decl`
  - Fix: remove the key
  - Severity: warning
  - Code action: easy. Delete the `key = value;` line. If it was the only
    key, the empty `config = {}` block is left in place.
  - Refs: src/config.h:39; CHANGELOG.md:300, :393; bpftrace emits:
    src/ast/passes/config_analyser.cpp:31
- [x] Renamed: `strlen` -> `max_strlen`, `no_cpp_demangle` ->
      `cpp_demangle`, `cat_bytes_max` -> `max_cat_bytes`, `map_keys_max` ->
      `max_map_keys`
  - Fix: use the new name
  - Severity: warning
  - Code action: easy. Replace the key name.
  - Refs: src/config.cpp:336; bpftrace emits:
    src/ast/passes/config_analyser.cpp:46
- [x] Environment-only: `btf`, `kernel_build`, `kernel_source`, `vmlinux`
  - Fix: set `BPFTRACE_<NAME>` in the environment instead
  - Severity: error
  - Code action: easy, but only removal. Delete the line; the message tells
    the user to set the variable in the environment instead.
  - Refs: src/config.cpp:345, :374
- [x] Unknown key
  - Fix: none; report "not a known configuration option"
  - Severity: error
  - Code action: moderate. Offer "did you mean" fixes for known keys that
    are within a small edit distance.
  - Refs: src/config.cpp:379

## Removed constructs (hints)

- [x] `sarg0`..`sargN`
  - Fix: `*(reg("sp") + <offset>)`
  - Severity: error
  - Code action: none. The stack offset depends on the CPU architecture.
  - Refs: CHANGELOG.md:792, :222; bpftrace emits:
    src/ast/passes/deprecated.cpp:66
- [x] `sym()`
  - Fix: `ksym()`
  - Severity: error
  - Code action: easy. Rename the function.
  - Refs: CHANGELOG.md:2376 (deprecated), :1653 (removed)
- [x] `stack`
  - Fix: `kstack`
  - Severity: error
  - Code action: easy. Rename the builtin.
  - Refs: CHANGELOG.md:2377 (deprecated), :1653 (removed)
- [x] Tracepoint `args->data_loc_<name>`
  - Fix: `args.<name>`
  - Severity: error
  - Code action: easy. Remove the `data_loc_` prefix and replace `->` with
    `.`.
  - Refs: CHANGELOG.md:1332
- [x] Multi-key `delete(@m[1], @m[2], ...)`
  - Fix: one `delete(@m, k)` per key
  - Severity: error
  - Code action: moderate. Split it into one statement per key, keeping the
    original indentation.
  - Refs: docs/migration_guide.md:118
- [x] `exit()` inside a loop
  - Fix: move it out of the loop
  - Severity: error
  - Code action: none. Moving it changes the control flow.
  - Refs: CHANGELOG.md:218; bpftrace emit location TBD

## Soft recommendations (hint severity)

- [x] `strncmp(a, b, n)` used only for equality
  - Fix: `==` / `!=`
  - Code action: none. When `n` is shorter than the strings, `==` gives a
    different result.
  - Refs: docs/stdlib.md:1587
- [x] `has_key(@m, k)` followed by reading `@m[k]`
  - Fix: `find(@m, k, $value)`, which does a single map lookup
  - Code action: none. Depends on the surrounding code.
  - Refs: docs/stdlib.md:455
- [x] Positional parameters `$1`, `$#`
  - Fix: named parameters with `getopt()`
  - Code action: none. A parameter name and default value have to be
    chosen.
  - Refs: docs/language.md:129
- [ ] Mixing BTF types with `#include`d header definitions
  - Fix: prefer BTF only
  - Code action: none.
  - Refs: docs/language.md:2314
- [ ] `pid` / `tid` used as 64-bit or init-namespace values
  - Fix: `(uint64)pid`, `pid(init)` / `tid(init)`
  - Code action: n/a. Which fix applies depends on why the value is used.
  - Refs: docs/migration_guide.md:36, :158
