# Session handover

## Current task: Task 13 — Windows `.msi` installer (seamly2d + seamlyme + SeamlyLayout, x64 + arm64)

**Branch:** `task13-msi-installer` (created from `run-seamlyLayout`, which is synced to origin `1b18a76a7` + the develop merge `1b457915a`). **Not yet committed, pushed, or PR'd.**

**Status:** implementation is **complete and verified locally**; remaining work is finishing two docs files, ticking the TODO checkboxes, then the commit → push → PR → CI → merge cycle.

### What is done & verified

- **Local debug build:** `scripts/sd.ps1` → exit 0.
- **Unit tests:** full suite **31,450 passed / 0 failed** across 24 suites, including the new `TST_SeamlyFamilyPaths` (7 passed).
- **x64 MSI build:** `scripts/smsi.ps1` produced `seamly-build-msi\x64\Seamly2D-x64.msi` (175.3 MB); `wix msi validate` passed with only the expected **ICE61** warning (a benign consequence of `AllowSameVersionUpgrades`).
- **MSI contents verified** via Windows Installer COM automation (scratchpad `inspect-msi.ps1`): platform `x64;1033`, UpgradeCode `{CBF4B5F1-C32C-4DBB-B385-3EE4A7B30658}`, exes in correct dirs (parents in `INSTALLFOLDER`, SeamlyLayout in `SEAMLYLAYOUTFOLDER`), three advertised Start-Menu shortcuts, `.sm2d`/`.smis`/`.smms` associations, HKLM registry rows, 1644 files, all runtime spot-checks (qwindows.dll, msvcp140.dll, QtWebEngineProcess.exe, xerces-c, default_settings.json, qt-source-notice.txt) present.
- **SeamlyLayout release build** built successfully (used as the MSI's layout staging input).

### Files created (all untracked, `??`)

| File | Purpose |
|---|---|
| `src/libs/vmisc/seamly_family_paths.h` / `.cpp` | `SeamlyFamilyPaths::locateSeamlyLayout(dir)` — finds `SeamlyLayout(.exe)` flat-beside-parent first, then `SeamlyLayout\` subdir (isFile-checked). "s"-prefix + GPLv3 header per rules. |
| `src/test/Seamly2DTest/tst_seamlyfamilypaths.h` / `.cpp` | `TST_SeamlyFamilyPaths`, 5 slots (empty / flat / subdir / flat-precedence / dir-named-like-exe-ignored), QTemporaryDir-based. |
| `packaging/windows/seamly-family.wxs` | WiX v6 source (install layout, shortcuts, associations, MajorUpgrade, ProgIds, HKLM InstallInfo). |
| `packaging/windows/license.rtf` | License summary shown in installer UI. |
| `packaging/windows/README.md` | Hands-on MSI build/test reference — **DONE**. |
| `scripts/smsi.ps1` | Staging + `wix build` + `wix msi validate` driver (Arch, Version, *Bin dirs, NoSeamlyLayout, WinDeployQt6, SkipValidation params). |
| `.github/workflows/windows-msi.yml` | Separate CI workflow, matrix x64/arm64 (arm64 = `-NoSeamlyLayout`); installs both Qt toolchains, builds parents (qmake/nmake) + SeamlyLayout (CMake), WiX v6, signs via jsign guarded on `SEAMLY_SIGNING_PROJECT_ID`. |

### Files modified (`M`)

- `src/libs/vmisc/vmisc.pri` — added `seamly_family_paths.cpp/.h` to SOURCES/HEADERS.
- `src/app/seamly2d/core/application_2d.cpp` — `seamlyLayoutFilePath()` now calls `SeamlyFamilyPaths::locateSeamlyLayout(QCoreApplication::applicationDirPath())` after the settings-override check; kept the hardcoded dev-build fallback; added the include.
- `src/test/Seamly2DTest/Seamly2DTest.pro` — registered new test SOURCES + HEADERS.
- `src/test/Seamly2DTest/qttestmainlambda.cpp` — include + `ASSERT_TEST(new TST_SeamlyFamilyPaths());`.
- `TODO_MIGRATE.md` — (working-tree) Task 13 intro reworded, Task 16/17/18 checkboxes `[x]`→`[X]`. **Task 13 subtask checkboxes are still all `[ ]` — must be ticked before commit** (see below).
- `future-todos.md` — pre-existing working-tree change (unrelated to Task 13).

### Key decisions (rationale in `packaging/windows/README.md`)

- **WiX v6** (not v7 — v7 requires accepting the OSMF EULA, error WIX7015; a policy call the project hasn't made). UI extension version must match core tool.
- **One bundled MSI per arch**, not per-app MSIs (the three are a family).
- **Install layout:** parents share one Qt runtime in `…\Seamly2D\`; SeamlyLayout gets its **own** Qt runtime in `…\Seamly2D\SeamlyLayout\` (built against a different Qt release; identical DLL names can't co-exist) — hence the `SeamlyFamilyPaths` discovery helper.
- **MSVC CRT deployed app-locally** (not merge modules / vc_redist chaining).
- **UpgradeCode `cbf4b5f1-c32c-4dbb-b385-3ee4a7b30658` is fixed forever**, shared by both arches. `MajorUpgrade` + `AllowSameVersionUpgrades`.
- **Version mapping:** MSI caps major ≤ 255, so `smsi.ps1` derives `(YYYY−2000).M.((D−1)·1440+HH·60+MM)` and stores the real `YYYY.M.D.HHMM` as `DisplayVersion` in `HKLM\SOFTWARE\Seamly\Seamly2D`.
- **arm64:** parents cross-compile like `ci.yml`'s windows matrix; **SeamlyLayout has no arm64 build yet**, so the arm64 MSI ships parents only (`-NoSeamlyLayout`).
- **User data untouched** on install/upgrade/uninstall (`%LOCALAPPDATA%\Seamly\<app>`, `%APPDATA%\Seamly\qt6_common.ini`, `C:\Users\<user>\seamly2d`).

## TODO / COMPLETED state

- `TODO_MIGRATE.md` still has **Task 13 with all 7 subtask checkboxes unchecked** ([TODO_MIGRATE.md:13-19](TODO_MIGRATE.md#L13-L19)). Six are fully done; the **verify** subtask (line 18 — clean-machine install/uninstall, arm64 hardware) is only partially satisfiable locally (COM inspection + validate done; no clean-VM or arm64-hardware run). Decide whether to check it with a caveat note or leave the whole task in `TODO_MIGRATE.md` pending real-hardware verification rather than moving to `COMPLETED.md`.
- `COMPLETED.md` — last entry added was **Task 20** (SeamlyLayout CI).

## Concrete next steps (resume here)

1. **Finish docs** (the only remaining implementation work):
   - `.github/README-BUILDS.md` — Windows section currently says under "Planned (Task 13)": a `.msi` installer, "Tooling decision pending (e.g. WiX)". Replace with the shipped WiX v6 design (bundled per-arch MSI, install layout, CRT app-local, version mapping, arm64/SeamlyLayout cross-compile story, signing).
   - `.github/workflows/README_WORKFLOWS.md` — add a "Windows MSI" workflow entry (mirror the SeamlyLayout CI entry style).
2. **Tick `TODO_MIGRATE.md` Task 13 subtasks** (with the verify caveat above); move Task 13 to `COMPLETED.md` only if you judge it complete.
3. **Commit** everything on `task13-msi-installer`.
4. **Push** to origin (`seamly/Seamly2D`) and **open a PR targeting `run-seamlyLayout`** — `gh` default repo is origin; **NEVER** the upstream `FashionFreedom/Seamly2D`.
5. **Watch CI** (`gh pr checks <pr> --watch`). Green → merge; fail → do not merge. Note the new `windows-msi.yml` is path-filtered and **will** trigger on this PR (it touches `packaging/windows/**`, `scripts/smsi.ps1`, the workflow file).
6. **Notify the user** of the outcome. After merge: update local `run-seamlyLayout` from origin, delete local `task13-msi-installer`.

## Gotchas seen this session

- **PowerShell 5.1 + `$ErrorActionPreference='Stop'`:** native-tool stderr (e.g. windeployqt6's optional-dependency warnings) becomes a terminating error. `smsi.ps1`'s `Invoke-Tool` relaxes to `'Continue'` and judges success by exit code only — keep that pattern for any new native-tool calls.
- **`msiexec /a` extract returned 1603** on this machine (cloud `G:\` drive interference in the log) — verify MSIs via the COM automation script instead (`scratchpad/inspect-msi.ps1`).
- **clangd PostToolUse diagnostics** on `application_2d.cpp` / `qttestmainlambda.cpp` are stale-noise (no compile DB for app/test sources) — ignore; the qmake build is the real check.
