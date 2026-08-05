# Migrate the C/C++ indexer from Clang 19 to Clang 22

## Context

Sourcetrail's optional C/C++ language package (`src/lib/lib_cxx`) is built on Clang/LibTooling, currently pinned to **LLVM/Clang 19.1.7**. We want to build and index correctly against **Clang 22** without regressing indexing behavior.

Decisions for this effort (from user):
- **Keep back-compatibility**: both Clang 19 *and* 22 must compile. Fixes are added as new `#if CLANG_VERSION_MAJOR >= N` branches alongside the existing guards — never delete the old branch.
- **Toolchain is already available locally** — point `Clang_DIR` at a local LLVM/Clang 22 build. No CI/release-artifact changes in this effort.
- **Scope: source + build only.** CI workflows, README/docs/skill version strings, `DOCUMENTATION.md`, and the release-artifact pipeline (`build.yml`, `appimage.yml`, `clang_build_*.yml`, GitHub release `2.0.0`) are **deferred** to a follow-up.

The migration risk is entirely in `lib_cxx`; nothing else links Clang. The single biggest source of behavioral regression is the parser integration tests, which assert on exact serialized symbol strings.

## Approach

This is fundamentally an **iterative compile-fix loop against local Clang 22**, seeded with the known break points below. The API deltas between 19 and 22 (LLVM 20/21/22) are real but can't all be predicted from memory — the compiler is the source of truth. Add version guards keyed on `CLANG_VERSION_MAJOR` (macro comes from `clang/Basic/Version.h`, already included at every guard site).

### Step 1 — Point the build at Clang 22 and configure
- Use a local preset (`CMakeUserPresets.json`, uncommitted) or override `Clang_DIR` to the Clang 22 build's `lib/cmake/clang/`, with `BUILD_CXX_LANGUAGE_PACKAGE=ON`. Existing wiring: `CMakePresets.json` `build_cxx` preset, `src/lib/lib_cxx/CMakeLists.txt:2` (`find_package(Clang REQUIRED)`).
- Verify `src/lib/lib_cxx/CMakeLists.txt:76-96` still resolves component lib names under LLVM 22 (`llvm_map_components_to_libnames`, the manual `LLVMX86AsmParser` / per-target `CodeGen`/`AsmParser` appends). Prefer the dylib paths (`LLVM_LINK_LLVM_DYLIB` / `CLANG_LINK_CLANG_DYLIB`, lines 73/99) which sidestep component renames.

### Step 2 — Fix compilation, one guard site at a time
Work file-by-file until `Sourcetrail_lib_cxx` links. Known / high-probability break points (all under `src/lib/lib_cxx/data/parser/cxx/`), each to be wrapped in a new `#if CLANG_VERSION_MAJOR >= N` branch preserving the existing 19 branch:

**Highest-confidence hard breaks (LLVM 21 `DiagnosticOptions` rework — no longer `IntrusiveRefCntPtr`):**
- `CxxDiagnosticConsumer.h/.cpp` — ctor takes `clang::DiagnosticOptions* diags` and forwards to `TextDiagnosticPrinter`. In 22 `TextDiagnosticPrinter` takes `DiagnosticOptions&`. Rework the ctor signature + base call under a guard.
- `ClangInvocationInfo.cpp` and `CxxParser.cpp` — every `IntrusiveRefCntPtr<DiagnosticOptions>` / `new clang::DiagnosticOptions()` / `ParseDiagnosticArgs` / `CompilerInstance::createDiagnostics` / `TextDiagnosticPrinter` construction site. Diagnostics engine setup changed ownership model.

**NestedNameSpecifier rework (LLVM ~21 — value-type, `TypeSpecWithTemplate` removed):**
- `name_resolver/CxxSpecifierNameResolver.cpp` — the `switch` on `NestedNameSpecifier::SpecifierKind` (`Identifier`/`Namespace`/`NamespaceAlias`/`TypeSpec`/`TypeSpecWithTemplate`/`Global`/`Super`) and the `getAsNamespace`/`getAsNamespaceAlias`/`getAsType` accessors. Likely the most involved single file.

**Existing guard sites to re-examine (already navigated the 15→16 break; verify 22 didn't move again):**
- `PreprocessorCallbacks.h:29` + `.cpp` — `InclusionDirective` signature (the only existing `>= 19` guard). Confirm `OptionalFileEntryRef` / `ModuleImported` params are unchanged in 22; add a `>= N` branch if not.
- `CxxAstVisitor.cpp:223,334` — `TemplateTypeParmDecl::getDefaultArgument`, `getTemplateArgsAsWritten`.
- `name_resolver/CxxDeclNameResolver.cpp:202,251` — `SubstTemplateTypeParmType::getReplacedParameter`, `NamespaceDecl` canonicalization.
- `name_resolver/CxxTypeNameResolver.cpp:119,168` — `template_arguments()` accessors.
- `utilityClang.cpp:61,134` — `TagTypeKind` scoped enum, `FileEntry` path helpers.
- `CanonicalFilePathCache.cpp:10,50` — `FileEntryRef`/`OptionalFileEntryRef`.
- `CxxAstVisitorComponentIndexer.cpp:662`, `CxxAstVisitorComponentBraceRecorder.cpp:114,152` — `Lexer::findNextToken` return type.

**Broad-surface files to watch (RecursiveASTVisitor CRTP override signatures can drift):**
- `CxxAstVisitor.h/.cpp` — ~80 `Traverse*`/`Visit*` overrides; a changed base signature silently stops overriding (no error) → missing index data. Watch for `-Woverloaded-virtual` / `-Winconsistent-missing-override` and behavioral gaps in tests.

### Step 3 — Run the indexer tests and reconcile output changes
Build with `ENABLE_INTEGRATION_TEST` (+ unit tests) and `BUILD_CXX_LANGUAGE_PACKAGE=ON`, then:
```
ctest --test-dir build -R "integration.lib_cxx\."
```
Suites: `CxxParserTestSuite`, `CxxParser14TestSuite`, `CxxParser17TestSuite`, `CxxIncludeProcessingTestSuite` (`tests/integration/lib_cxx/`). These assert **exact** serialized symbol strings — including lambda/anonymous names keyed by source location and template-argument printing — which are the outputs most perturbed by a Clang major bump.

For each failure, decide deliberately:
- **Acceptable churn** (e.g. Clang changed how it prints a template arg or names an implicit lambda) → update the inline `EXPECT_THAT(..., L"...")` literal. There are **no golden files**; assertions are inline string literals.
- **Real regression** (a symbol/reference no longer recorded, wrong location, dropped edge) → a `Visit*` override likely stopped matching a changed base signature, or a name resolver lost a case. Fix the code, not the test.

Also run the `src/lib/lib_cxx/tests/` unit suites (compilation-database / include parsing — not AST-sensitive) as a sanity check.

## Critical files
- `src/lib/lib_cxx/CMakeLists.txt` — Clang discovery + component/dylib linking (verify only).
- `src/lib/lib_cxx/data/parser/cxx/CxxDiagnosticConsumer.{h,cpp}`, `ClangInvocationInfo.cpp`, `CxxParser.cpp` — DiagnosticOptions ownership break.
- `src/lib/lib_cxx/data/parser/cxx/name_resolver/CxxSpecifierNameResolver.cpp` — NestedNameSpecifier rework.
- `src/lib/lib_cxx/data/parser/cxx/PreprocessorCallbacks.{h,cpp}` — InclusionDirective guard.
- `src/lib/lib_cxx/data/parser/cxx/CxxAstVisitor.{h,cpp}` + `CxxAstVisitorComponent*.cpp` — visitor override surface.
- Remaining `CLANG_VERSION_MAJOR` guard sites listed in Step 2.
- `tests/integration/lib_cxx/CxxParser*TestSuite.cpp` — expected-string reconciliation.

## Verification
End-to-end, not just tests:
1. **Compile**: `Sourcetrail_lib_cxx` links against Clang 22 (and still against 19 if a 19 build is handy — the guards must not break the 19 path).
2. **Automated**: `ctest --test-dir build -R "integration.lib_cxx\."` and the `src/lib/lib_cxx/tests` unit suites all pass (with justified expected-string updates).
3. **Real indexing run**: launch the built app from `bin/`, create/refresh a small C++ project (or reuse a test-data source group), and confirm the graph/symbols/references/call graph populate correctly — the ultimate check that visitor overrides still fire under Clang 22.

## Deferred (explicitly out of scope here)
Version-string bumps and the release-artifact pipeline: `README.md`, `.claude/skills/cmake/SKILL.md`, `DOCUMENTATION.md` (also stale "Clang 11.0.0"), `.devcontainer/Dockerfile`, and `.github/workflows/{build.yml,appimage.yml,clang_build_ubuntu.yml,clang_build_window.yml}` (+ publishing `clang-22.x` artifacts to the GitHub release the downloader points at). Track as a follow-up before shipping.