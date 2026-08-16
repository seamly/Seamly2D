# Session handover

Only the **current** state lives here. Completed tasks are written up in
`project-docs/TODO_COMPLETED.md`, and the reasoning behind shipped decisions
lives beside the code it governs — for Windows packaging that is
`scripts/packaging/windows/README.md` and `INSTALL_DECISION_FLOW.md`. Do not
re-accumulate finished-session narrative in this file.

## PICK UP HERE (2026-08-15, the MSI installs end to end)

**The rebuilt `dev-latest` MSI completed a full interactive install.**
`InstWinX64.1.6` is closed. All ten pages drew. The data root composed as
`C:\Users\susan\SeamlyData\`, so the trailing-backslash fix holds. Screenshots
are in `project-docs/Install*-Screenshot 2026-08-15 *.png`.

**Next: confirm the three follow-up fixes in a real install.** They are pushed
but only the two `.wxs` ones are verified, and only statically. Download the
rebuilt `dev-latest` MSI and check the data-root page reads "Put the SeamlyData
folder in:" with no ampersand.

### seamly2d.exe and seamlyme.exe went missing after a successful install

**Unexplained. Watch for it.** After the 20:57 install the two parent
executables were absent from `C:\Program Files\SeamlyApps`, while
`SeamlyLayout.exe` and all 1600 harvested files were present.

Ruled out, each by direct evidence:

- The package. `msiexec /a` on that exact MSI extracts all three exes.
- The authoring. `File`, `Component` and `FeatureComponents` rows are correct
  and unconditional; all three exes sit in `WixDefaultFeature`/`INSTALLFOLDER`.
- The sequence. `RemoveExistingProducts` 1401, `RemoveFiles` 3500,
  `InstallFiles` 4000.
- The legacy remover. No `NSIS_Seamly2D` key on the machine, so
  `SEAMLYLEGACYINSTALLDIR` was empty and its component never installed.
- Defender. No detection, no quarantine event.

Windows Installer registered both components as `Installed: Local` with the
right key paths, and logged the install as successful. `MsiGetComponentPath`
returned empty, which is what it does when a registered key path is gone from
disk.

Fixed by an elevated repair: `msiexec /i <msi> REINSTALL=ALL
REINSTALLMODE=vomus`. The repair log says `No existing file`, confirming the
files were genuinely absent rather than skipped by file versioning. A silent
repair without `-Verb RunAs` fails with 1625.

**If it happens again, get a log — that is the missing evidence:**
`msiexec /i seamly-x64.msi /l*v %USERPROFILE%\Desktop\seamly.log`

One contributing factor to remove: the install ran the MSI straight out of
Explorer's zip temp folder
(`InstallSource: ...\Temp\<guid>_seamly-x64.msi (3).zip.c9c\`). Explorer deletes
that folder when the zip window closes. Extract the MSI first.

Two housekeeping items in `project-docs/`:

- `Installer-6-Screenshot 2026-08-15 162124.png` is the *old* "ended
  prematurely" page. It sits in the middle of the new sequence. Delete it.
- The screenshots are untracked. Decide whether they belong in the repository.

### Three defects fixed on 2026-08-15 (keep the lessons)

- **`Wix4RemoveFoldersEx` runs at sequence 799, before `CostInitialize`.** Its
  `RemoveFile` rows have to exist in time for costing. Any property it reads
  must therefore be set earlier than that, and cannot expand a directory
  property — those are unresolved until `CostFinalize`. `SEAMLYLEGACYSTARTMENU`
  was set at 1001 with `[AppDataFolder]`; it is now set after `AppSearch` with
  `[%APPDATA]`. **Nothing failed and nothing logged.** The legacy Start Menu
  folder was simply still there.
- **`NoPrefix="yes"` turns accelerator parsing off**, so an `&` in that label
  prints. Removed from `FolderLabel`.
- **SeamlyLayout logged into the install directory on Windows.** Now the
  `AppConfigLocation` root, like macOS and the AppImage.

**`smsi_check_authoring.ps1`: `Get-MsiRows` returns `, $rows`. Assign it
directly — never `@(Get-MsiRows ...)`.** The wrapper gives an array holding an
array. Single-row queries still work, because PowerShell unwraps a one-element
array on a cast or member access, so the trap stays hidden until a query first
returns more than one row. 120 assertions pass.

**Rust is not installed on this machine.** `cargo`, `rustc` and `rustup` are all
absent, so `src/app/seamlylayout/build.ps1` fails in Corrosion's `FindRust`.
Nothing in SeamlyLayout builds locally until Rust is installed. CI is the only
verification for its C++ and Rust changes.

### The 2343 defect (fixed, keep the lesson)

Fixed and pushed (`a843503ab7..6bde38b1b3`).

2343 is "specified path is empty". `SeamlyDataDirDlg`'s path box carried
`Indirect="yes"`. **An indirect `PathEdit` reads its property to get the NAME of
the property that holds the path.** Stock `InstallDirDlg` is indirect only
because `WIXUI_INSTALLDIR` holds the string `INSTALLFOLDER`. `SEAMLYDATAPARENT`
holds the path itself, so the lookup asked for a property named
`C:\Users\<user>\`, found nothing, and aborted the install while the page was
being created. Remember this before copying an `Indirect` attribute off a stock
dialog.

Two more defects fixed on the way, both on the same page:

- Next had no `SetTargetPath`, so an edited parent never reached the Directory
  table and `[SEAMLYDATAROOT]` would not have recomposed. It runs before
  `NewDialog`, conditional on a non-empty property (the other route to 2343).
  Deliberately no `CheckTargetPath` — the data root may be a cloud or removable
  drive.
- The `SEAMLYDATAPARENT` default had no trailing backslash. Windows Installer
  appends a child directory verbatim, so `C:\Users\me` gave
  `C:\Users\meSeamlyData`.

`smsi_check_authoring.ps1` gained two assertions and now runs 118. Verified with
a link-only `wix build` over a stub staging tree.

## Earlier (2026-08-15, the MSIs publish to a rolling pre-release)

**Task InstWinX64.0.3 is done, and it needs one real CI push to prove it.**
`ci.yml` gained a `publish-windows-dev` job. Every push to `run-seamlyLayout`
deletes the `dev-latest` release, recreates it on the pushed commit, and uploads
`seamly-x64.msi` and `seamly-arm64.msi` as raw release assets.

- **The task as written was impossible.** GitHub serves every workflow artifact
  as a zip. `actions/upload-artifact` has no option to return a bare file, so
  the artifact named `seamly-x64.msi` downloads as `seamly-x64.msi.zip`. A
  release asset is the only raw `.msi` GitHub hands back. Do not reopen this.
- The job deletes and recreates instead of editing, because GitHub pins a tag to
  its creation commit and no `gh` command moves it.
- It depends on `windows-msi` only, and carries no Linux or macOS file.
- The versioned `publish` job is untouched.
- **Untested on a runner.** Watch the first run for two things: the
  `gh release delete ... || true` on the very first push, which has no
  `dev-latest` to delete, and the `GITHUB_TOKEN` permission (workflow-level
  `contents: write` should cover it).

**`TODO_INSTALLER_WIN_X64.md` had two tasks numbered `InstWinX64.0.3` and two
numbered `InstWinX64.0.4`.** The Application-preferences pair is renumbered to
`0.5` and `0.6`. Check for a collision before adding a task number to that file.

## Earlier (2026-08-15, the local build and test scripts are deleted)

**`scripts/sb.ps1`, `scripts/sd.ps1` and `scripts/st.ps1` are gone.** The user
decided the family needs no local release build, no local debug build, and no
local test runner. `ci.yml` is the verification path for seamly2d and seamlyme.
SeamlyLayout keeps its own local scripts (`qd.ps1`, `build.ps1`) and its local
`ctest`/`cargo test`.

Consequences to keep in mind:

- **A skip-ci push now defers *all* verification for the parents.** Nothing runs
  locally to catch a broken build first. **The user kept the skip token as the
  default anyway (2026-08-15)** and verifies releases with a manual
  `gh workflow run ci.yml --ref run-seamlyLayout`. `ci.yml` already carries a
  `workflow_dispatch` trigger. Do not reopen this.
- **`smsi.ps1` has no local producer for its input trees.** It only packages.
  Build the trees by hand, or let `ci.yml`'s `windows-msi` job do the whole job.
- **The deployed-runtime FileVersion check went with `sb.ps1`/`sd.ps1`.** Read
  `Qt6Core.dll`'s FileVersion by hand before trusting a hand-built tree.
- `scripts/seamly2d-debug/` (2.1 GB) was deleted from disk, and its `.gitignore`
  entry with it, along with the older `scripts/seamly2d-build-debug/` spelling.
  `scripts/seamly-msi/` and `build/` stay ignored by name.

## Earlier (2026-08-15, smsi.ps1 is CI-only)

**`smsi.ps1` no longer builds locally, no longer builds a two-app package, and
no longer detects anything.** Three removals, all done:

1. **Local-build mode.** `-Version`, `-Seamly2DBin`, `-SeamlyMeBin` and
   `-WinDeployQt` are `Mandatory`. `Find-WinDeployQt6` (CMakeCache + `C:\Qt`
   scan) is deleted; `Find-CrtDirectory` reads `VCToolsRedistDir` only, with no
   Visual Studio scan. Each removed fallback could ship a runtime nothing in the
   package was built against.
2. **`-NoSeamlyLayout`.** Gone, with the `IncludeSeamlyLayout` define, the two
   `<?ifdef?>` guards in `seamly-family.wxs`, and `-ExpectSeamlyLayout` in
   `test_msi_authoring.ps1` (its SeamlyLayout assertions are unconditional now).
   Both architectures have shipped all three apps since 2026-08-11.
3. **`windeployqt6`.** The project uses the unsuffixed `windeployqt` everywhere:
   the parameter is `-WinDeployQt`, `ci.yml` passes
   `"$env:QT_ROOT_DIR\bin\windeployqt.exe"`, and
   `src/app/seamlylayout/packaging/windows/build_installer.ps1` was renamed to
   match. A Qt 6 kit ships both names, so this is a naming choice, not a
   behaviour change.

**Verified with the stub-staging-tree trick** (see below): `wix build` clean,
`wix msi validate` clean except the expected ICE61, `test_msi_authoring.ps1`
all assertions pass including the three SeamlyLayout ones. The next real CI run
is what proves the `-WinDeployQt` rename against the runners' Qt kits — the
change is untested on a runner.

## Earlier (2026-08-12, custom dialog set)

**Tasks InstWinX64.0.2 and InstWinX64.1.1–1.5 are done.** The user confirmed the
x64 MSI builds in CI, and `seamly-family.wxs` now defines its own dialog set
instead of using `WixUI_InstallDir`.

**The SpawnDialog blocker is gone.** A dialog set owns every `NewDialog` row; a
stock dialog carries only its own internal events. So replacing the set — rather
than publishing against it — makes every transition ours, and the Order 4
`NewDialog VerifyReadyDlg` row that could not be excluded no longer exists.

Chain: `WelcomeDlg` → `LicenseAgreementDlg` → `SeamlyPreviousInstallDlg` (only
when an earlier install is found) → `InstallDirDlg` → `SeamlyDataDirDlg` →
`SeamlyDataMigrateDlg` → `SeamlyShortcutsDlg` → `VerifyReadyDlg` → `ProgressDlg`
→ `ExitDialog`. Back reverses every arrow.

**Two findings worth keeping.**

1. `DialogRef` order decides the `InstallUISequence` numbers of `ResumeDlg`,
   `WelcomeDlg`, `MaintenanceWelcomeDlg` and `ProgressDlg` (1296–1299). Listing
   `WelcomeDlg` first put it at 1296 and pushed `ResumeDlg` to 1298, which would
   show the welcome page to a user resuming an install. The order is now
   commented as load-bearing and the test pins the numbers.
2. `WixUI_Common` supplies the bitmaps but **not** `WixUI_Font_Normal`,
   `WixUI_Font_Bigger` or `WixUI_Font_Title`. A custom set has to define them,
   plus `DefaultUIFont`, `WIXUI_INSTALLDIR` and `ARPNOMODIFY`.

**Local verification is now cheap and was done.** A stub staging tree (three
empty `.exe` files and two files for the runtime) links the real authoring in
seconds: `wix build` clean, `wix msi validate` clean except the expected ICE61,
`test_msi_authoring.ps1` 115 assertions pass. It proves the authoring, not the
product.

**InstWinX64.1.7 is done too.** `INSTALL_DECISION_FLOW.md` and
`scripts/packaging/windows/README.md` carry the new page order, and the
"SeamlyShortcutsDlg never displays" defect note is gone. The README also claimed
the old NSIS installation is never removed automatically; Setup has removal
components for it, so that claim was corrected.

**Next: InstWinX64.1.6** — an interactive install on the test laptop. Every page
must display, in order, and Back must return to the previous page. It is the
only part of Task InstWinX64.1 that local checks cannot cover.

## CI: one workflow (2026-08-12)

`.github/workflows/seamlylayout-ci.yml` is deleted. **`ci.yml` is the only
workflow that builds the family on GitHub.** It already built seamlyLayout in
the `windows-msi` job, so the second workflow duplicated the build and carried a
second `QT_VERSION`.

**Coverage lost, on purpose:** seamlyLayout's `ctest` and `cargo test
--workspace` suites no longer run in CI, and seamlyLayout is no longer built on
Linux. Run both locally before merging seamlyLayout work. To restore the
coverage, add the two test steps to `ci.yml`.

Docs updated: `README_WORKFLOWS.md`, `README-BUILDS.md`, `CLAUDE.md`,
`src/app/app.pro`, `UNIT_TEST_COMMANDS.md`, `TODO_MIGRATE.md`,
`TODO_RENAME_SETTINGS_FILES_CLASSES.md`. `TODO_COMPLETED.md` keeps the Task 20
entry as the record of what was built at the time.

## Earlier (2026-08-11, data-root append + SpawnDialog investigation)

> The SpawnDialog blocker described below was fixed on 2026-08-12 by the custom
> dialog set. Kept for the reasoning that led there.

**Done: the data root appends a fixed leaf.** `SEAMLYDATAPARENT` is what the
user picks (default `%USERPROFILE%`); `SEAMLYDATAROOT` is that parent plus a
fixed `SeamlyData` leaf. `E:\` gives `E:\SeamlyData`. Setting `SEAMLYDATAROOT`
directly still overrides the composition. 96 authoring assertions pass, two new
ones pinning the composition. Merged and pushed (`4a8c0a07dc..37fb33f1f9`).

**Not done: the SpawnDialog defect.** The built-in `InstallDirDlg` Next rows
occupy Orders 1, 3 and 4. Below the Order 4 `NewDialog` only 0 and 2 are free —
two slots for three pages. The ties are forced. No arrangement of `Ordering`
values can fix them; the mechanism has to change, not the numbers.

Orders 5-7 was tried and **reverted**. It removes every tie, but it contradicts
a deliberate assertion in `test_msi_authoring.ps1` ("SpawnDialog must have a
lower Ordering than every NewDialog"), and only an interactive install can
settle which reading is right. The tree is back to Orders 1-3, with the finding
in the comment beside the `Publish` rows.

**Also this session:** `.claude/settings.json` gained the nuget and dotnet
sandbox domains, so the WiX toolchain can be installed. `Bash`/`PowerShell`
rules were already maximal; `permissions.defaultMode` remains unset.

### Earlier the same day

## PICK UP HERE (2026-08-11, installer directories session)

**Tasks InstWinX64.1.1 and 1.2 — authored, and 1.2 is BLOCKED on a defect that
was already in the tree.** Read the blocker note in `TODO_INSTALLER_WIN_X64.md`
before continuing.

**1.1 was mostly already built.** `C:\Program Files\SeamlyApps` was the existing
default (`ProgramFiles64Folder` + `Name="SeamlyApps"`), and every shortcut,
association and registry value already resolved through `[INSTALLFOLDER]`. The
user asked about `Program Files (x86)`, then decided to keep the current path —
correctly, since that tree is for 32-bit programs and every binary here is x64
or arm64. What was genuinely missing was the cloud-folder rejection (1.1.3),
now a `Launch` condition covering OneDrive, Dropbox, Google Drive, iCloud and
Box Sync. It is a launch condition rather than a dialog check because that is
the only form that also blocks a silent `/qn` install.

**1.2 was built to "installer does everything"** — the user chose that split
over "installer records, apps copy". So the MSI now carries `SEAMLYDATAROOT`,
`SEAMLYCOPYUSERDATA`, two dialogs, a registry breadcrumb, and a deferred
impersonated custom action running `seamly_copy_user_data.ps1`.

**THE BLOCKER.** The two new dialogs use `SpawnDialog` from `InstallDirDlg`'s
Next — the same mechanism as `SeamlyShortcutsDlg`, which
`INSTALL_DECISION_FLOW.md` already records as **never displaying**. Dumping the
built MSI's `ControlEvent` rows proved there is no alternative: the built-in
`NewDialog VerifyReadyDlg` is at Order 4 with condition `1`, so no competing
`NewDialog` can be excluded. The dump also exposed two ordering collisions this
task introduced (`SeamlyDataDirDlg` ties with `CheckTargetPath` at Order 1;
`SeamlyShortcutsDlg` ties with `SetTargetPath` at Order 3). The full chain is
transcribed in a comment beside the `Publish` rows in `seamly-family.wxs`.

Consequence: the **properties work** (an unattended install passing
`SEAMLYDATAROOT` / `SEAMLYCOPYUSERDATA` behaves correctly, defaults apply
otherwise) but the **interactive prompting does not**. 1.2.1-1.2.3 are left
unticked on purpose. Fixing the SpawnDialog defect is the tracked subtask in
`TODO_MIGRATE.md` and must come first.

**Why there is no rollback action for the copy** — a deliberate deviation from
the option text the user picked, flagged at the time and worth keeping: undoing
the copy could only mean deleting files from a folder that may already have held
the user's work, and nothing can tell those apart. The copy is additive-only and
never overwrites, so nothing is left inconsistent. `Return="ignore"` likewise: a
file-copy problem must not roll back a good program install.

**Verification done locally.** `wix build` + `wix msi validate` (with the same
`-sice ICE43 -sice ICE57` the real build uses) are clean, only the expected
ICE61. `test_msi_authoring.ps1` gained ~25 assertions and all 94 pass. The copy
script was run against a real tree: subfolders preserved, an existing
destination file left byte-for-byte, source untouched, second run copies 0.

**MACHINE STATE CHANGED OUTSIDE THE REPO.** The user installed .NET SDK 9
(`Microsoft.DotNet.SDK.9`, 9.0.316) and I installed the WiX v6 global tool
(`wix` 6.0.2) plus `WixToolset.UI.wixext` and `WixToolset.Util.wixext` at the
matching version. `dotnet` is NOT on `PATH` — prepend
`$env:USERPROFILE\.dotnet\tools` and `$env:ProgramFiles\dotnet`. This makes a
`.wxs` change checkable in seconds instead of a ~50-minute CI round trip. Undo
with `dotnet tool uninstall --global wix`.

**One open question for the user**, recorded in the task file: `SEAMLYDATAROOT`
holds the whole path, so choosing `E:\` gives `E:\`, not `E:\SeamlyData`.
Auto-appending the leaf would turn a typed `E:\SeamlyData` into
`E:\SeamlyData\SeamlyData`. Confirm which is wanted.

### Earlier the same day

## PICK UP HERE (2026-08-11, later session)

**Task InstWinX64.1.3.2 is done — `windows-msi.yml` is deleted.** `ci.yml`'s
`windows-msi` matrix job already built both architectures and fed `publish`, so
the packaging-only workflow only duplicated the work: its push trigger on
`scripts/packaging/windows/**` built both MSI packages a second time on every
`.wxs` or `smsi.ps1` edit. Its copy of the build steps had also drifted — it
signed `Seamly2D-<arch>.msi`, a name `smsi.ps1` has never written, so that
signing step had never touched a real file.

**Consequence to expect:** an edit under `scripts/packaging/windows/` now runs
the full ~50-minute suite instead of a path-filtered packaging job. That is the
accepted trade for one copy of the steps.

**Over twenty references had to be redirected, not four.** They were spread
across `.github/README-BUILDS.md`, `.github/workflows/README_WORKFLOWS.md`,
`common.pri`, `scripts/sb.ps1`, `scripts/packaging/windows/` (README.md,
README_WINDOWS_BUILD.md, smsi.ps1, seamly-family.wxs, test_msi_authoring.ps1),
`CLAUDE.md`, `seamlylayout-ci.yml`, `qt-arm64-module-probe.yml`, and four
`TODO_*.md` files. Completed-task records in `TODO_COMPLETED.md`,
`TODO_INSTALLER.md` and `TODO_INSTALLER_WIN_ARM64.md` keep the old name on
purpose — they describe what was true at the time.

**Stale claims corrected while redirecting.** Several packaging documents still
said the arm64 MSI ships the parents only (`-NoSeamlyLayout`) and that arm64
cross-compiles with `host-qmake`. Both are false since 2026-08-11: all three
apps build natively on `windows-11-arm`. `TODO_CODE_SIGNING.md` also cited a
`ci.yml` step named "Print installer signature" that went with the NSIS
`windows` job; CodeSign.1.5 now says the step has to be written, not mirrored.

**`qt-arm64-module-probe.yml` is also deleted** (by the user, same session). It
had answered its question — Qt 6.11.1 does publish an arm64 WebEngine — and the
subtask asking to re-run it at every Qt bump was removed first. Four documents
still pointed at it, two in the present tense; each now carries the `aqt
list-qt` command the workflow ran, so the check survives the file:
`aqt list-qt windows_arm64 desktop --modules <version> <arch>`, and the same for
the `windows` host.

`TODO_CODE_SIGNING.md` was updated by the user in the same session.

**Also this session:** `ci.yml`'s `paths-ignore` gained `.claude/**` and
`.vscode/**`. A push of documentation plus a `.claude/settings.json` edit had
started the full suite, because the filter skips a push only when *every* file
it carries matches. `CLAUDE.md`'s docs-only exception was corrected at the same
time — it told the reader to commit locally and never push, and it wrongly
counted `.txt` and `.svg` as documentation (`CMakeLists.txt` is a build input;
an `.svg` can be a compiled Qt resource).

**Verification status:** no code changed, so there was no build to run. The
edited PowerShell scripts and `seamly-family.wxs` were parse-checked. The
`ci.yml` YAML edit is unverified locally — the push that carries it is the
first real check, and the skip token was deliberately omitted for that reason.

### Previous session (2026-08-11)

**Task InstWinX64.0 — the x64 MSI builds clean on CI.** Runs `31461308276`
(CI) and `31461308379` (Windows MSI) on `361b743fa0` both finished **green**,
including `Windows: Build MSI (x64)`. `wix msi validate` passed,
`test_msi_authoring.ps1` passed, and `seamly-x64.msi` (163.5 MB) uploaded with
all three apps in it. So the three changes those runs were the first to
exercise — the edited `ci.yml`, the repaired authoring assertions, and the
Nsis→Legacy rename — are all verified. Nothing in `ci.yml` or the packaging
scripts needed a fix. Details, including the two harmless warnings (ICE61 from
`AllowSameVersionUpgrades`, and windeployqt's missing `Qt6SerialPort.dll` for
the unused NMEA plugin), are written up under Task InstWinX64.0 in
`TODO_INSTALLER_WIN_X64.md`.

**The held-back commit is pushed.** `a0e70635a7` and `064a1e49dd` went to
origin once those runs finished; `run-seamlyLayout` is at `064a1e49dd` on both
sides. That push started runs `31538442167` (CI) and `31538442086` (Windows
MSI) — a re-verification of a comment-only `.wxs` change, so a failure there
would be new and unrelated to the MSI authoring.

**The one thing left in Task InstWinX64.0** is its second subtask: the user has
to confirm the x64 `.msi` built without error. Only then does the task move to
`TODO_COMPLETED.md`.

Everything below is the state as shipped this session.

## Current state (2026-08-11): CI cost control + MSI authoring check repaired

Three changes merged to `run-seamlyLayout`; the branch was force-pushed once
(see the skip-token gotcha below for why).

**1. CI no longer runs on every push.** `ci.yml` gained a top-level `concurrency`
(cancel-in-progress) and a `paths-ignore` for `**.md` / `project-docs/**` /
`LICENSE` on its push trigger (since extended with `.claude/**` and
`.vscode/**`); `seamlylayout-ci.yml` already had both. `CLAUDE.md`'s task workflow now merges with `--no-ff` (step 8)
and puts the skip token in the merge commit by default (step 9), omitting it
when the task touched `.github/workflows/**`, `scripts/packaging/**`,
`*.pro`/`CMakeLists.txt`/`Cargo.toml` or platform-specific code — the things the
local debug build could not verify. That script is gone since 2026-08-15, so a
skip-ci push now defers verification of everything, not only those paths.
Accumulated skips get cleared with `gh workflow run ci.yml --ref run-seamlyLayout`
before a milestone.

**GOTCHA that cost a push:** GitHub matches the skip token anywhere in the head
commit message, including a sentence *saying the token is absent*. The first
version of that merge commit explained the new rule, contained the literal
token, and skipped the very run it was describing — only CodeQL (default setup,
not governed by the token) ran. Both commits were rewritten to spell it
"skip token" in prose and the branch was force-pushed with `--force-with-lease`.
Never write the literal token in a commit message.

**2. The MSI authoring check was failing, and not because of anything above.**
`546e9d5def` (2026-08-03, "fixed issues in build files related to directories,
filenames, etc.") hand-edited `seamly-family.wxs` and left
`test_msi_authoring.ps1` asserting the old values, so three assertions failed on
every push since. Fixed in the `.wxs`, test left alone, because the test encodes
Task 51's documented requirements and `SeamlyApps` is the name used by
`README-BUILDS.md`, `scripts/packaging/windows/README.md`,
`INSTALL_DECISION_FLOW.md`, `TODO_INSTALLER_WIN_X64.md` and the Task 51 test kit:

- `INSTALLFOLDER` `Name="Seamly"` → `Name="SeamlyApps"`.
- `UserDataText` names the user-data folder again (`C:\Users\your name\seamlyData`).
- `NsisText` (now `LegacyInstallText`, see below) regained the deleted sentence
  "Nothing else in that folder is kept, so move anything of your own out of it
  before continuing." That one was a real data-loss warning, not just a test
  mismatch — the dialog still says Setup removes the whole legacy *program
  directory*. Kept to 338 chars so it fits the unchanged `Height="50"` control;
  the pre-`546e9d5def` text was 337.

Verified locally by parsing the `.wxs` and running the six assertions' regexes
against the real attribute values (all pass, XML well-formed). The MSI itself
still only builds on CI.

**3. `Nsis` identifiers renamed to `Legacy`.** NSIS is a build tool this project
stopped using when the `windows` job was retired, so identifiers named for it
read as though the package still produces one. What they actually name is the
pre-MSI Seamly2D already sitting on a user's machine, which outlives the tool
that authored it. Renamed across `seamly-family.wxs`, `test_msi_authoring.ps1`,
both copies of `test_msi_install.ps1` and `INSTALL_DECISION_FLOW.md`:

| was | now |
|---|---|
| `SEAMLYNSISUNINSTALLSTRING` / `SEAMLYNSISINSTALLDIR` / `SEAMLYNSISSTARTMENU` | `SEAMLYLEGACY…` |
| `SeamlyNsisUninstallStringSearch` / `SeamlyNsisInstallDirSearch` | `SeamlyLegacy…` |
| `RemoveNsisProgramFiles` / `RemoveNsisRegistryKeys` | `RemoveLegacy…` |
| `RemovedNsisInstallDir` / `RemovedNsisRegistry` (registry breadcrumbs) | `RemovedLegacy…` |
| `NsisText` | `LegacyInstallText` |
| `Get-NsisInstallDir`, `$state.NsisInstallDir` | `Get-LegacyInstallDir`, `…LegacyInstallDir` |

**Two things deliberately NOT renamed.** `SOFTWARE\NSIS_Seamly2D` is a real key
name the old installer wrote — renaming it would make the MSI stop finding the
product it exists to remove. And prose still says NSIS where it names the actual
historical product; only identifiers moved. A `NAMING:` comment in the `.wxs`
records both rules.

`SecureCustomProperties` needed no manual edit — WiX generates it from
`Secure="yes"`, and `test_msi_authoring.ps1` asserts both renamed properties
appear in it.

**DECIDED with the user 2026-08-11: `RemovedLegacyInstallDir` and
`RemovedLegacyRegistry` are frozen — never rename them again.** They are
component KeyPaths on components with no explicit `Guid`, so WiX derives the
component GUID from the Id plus the value name. This rename spent that change
once (on upgrade the old value is removed and the new one written, which is
correct); repeating it leaves users' machines accreting orphaned breadcrumbs
under `SOFTWARE\Seamly\Seamly2D`. Recorded as `FROZEN NAME` comments at both
components in `seamly-family.wxs` — that is commit `a0e70635a7`, the unpushed
one named at the top of this file.

Verified locally: `.wxs` parses as XML, every `Legacy` identifier referenced by
`test_msi_authoring.ps1` exists in the `.wxs`, the six repaired assertions still
pass, and all four `.ps1` files parse with zero errors. The authoring check
itself runs only on CI.

## Superseded (2026-08-10): arm64 MSI build fixed — `windeployqt --qtpaths` removed

**Branch `task-arm64-windeployqt`, off `run-seamlyLayout`.**

**The failure.** `Windows: Build MSI (arm64)` died in the `nmake` leg:

```text
windeployqt.exe --qtpaths ...\msvc2022_arm64\bin\host-qtpaths.bat bin\seamlyme.exe
Error: "...\bin\host-qtpaths.bat" does not exist.
NMAKE : fatal error U1077 ... return code '0x1'
```

**Root cause.** `host-qtpaths.bat` is generated only for a **cross-compiled** kit
(`win64_msvc2022_arm64_cross_compiled`), whose x64 `windeployqt` cannot infer an
arm64 target's paths. Commit `fba962c4d8` moved arm64 onto the native
`windows-11-arm` runner with `win64_msvc2022_arm64`, where that wrapper does not
exist — but the `win32-arm64-msvc` block in the `.pro` files still passed
`--qtpaths` unconditionally.

**Fix — one shared qmake helper, `deployQtRuntime()` in `common.pri`.** All three
MSVC targets (`seamly2d.pro`, `seamlyme.pro`, `Seamly2DTest.pro`) had their own
hand-maintained copy of the windeployqt post-link block, which is exactly how
they drifted. They now all call the one helper, which runs
`qtPrepareTool(WINDEPLOYQT, windeployqt)` and invokes it **bare** — identically
for `win32-msvc` and `win32-arm64-msvc`. No probe, no `--qtpaths`: every Windows
leg is native, so `windeployqt` always belongs to the kit being deployed and
resolves its own paths. **Restore `--qtpaths` only if a cross kit is ever
reintroduced** (the comment block in `common.pri` says so).

**Also fixed, found on the way:** `tst_svgcomponenttags.cpp` called
`VLayoutPiece::SetCountourPoints()`, which does not exist (typo, and no such
setter) — it broke the whole `Seamly2DTest` target. The real setter behind
`getContourPoints()` is `setMainPathPoints()`. Pre-existing since `0dcdc3d35d`;
CI's MSI legs pass `CONFIG+=noTests` so they never hit it, but `linux-test` runs
`make check` on PRs and would have.

**Verified locally:** `scripts/sd.ps1` green; all three targets post-link
`C:\Qt\6.11.1\msvc2022_64\bin\windeployqt.exe bin\<app>.exe` with `Qt6Cored.dll`
+ `platforms\` deployed beside each exe; `scripts/st.ps1` = **32139 passed, 0
failed across 25 suites**. The arm64 path itself is **only provable by a CI
run** — no arm64 cross tools on this PC.

**Docs brought in line with the native-runner reality** (they still described
arm64 as cross-compiled and two-app): `README-BUILDS.md`,
`README_WORKFLOWS.md`, `README_WINDOWS_BUILD.md`, `ci.yml` (one comment was also
truncated mid-sentence), `windows-msi.yml`, and `TODO_INSTALLER_WIN_ARM64.md` —
where **InstWinArm64.1 is now DONE** (only .1.5, the three-app authoring-test
re-run, is open) and **InstWinArm64.3 is DROPPED**: the from-source Qt WebEngine
build existed solely to work around "Qt ships no arm64 WebEngine", which is Qt
6.8-era and false for 6.11.1.

**Next step:** push and confirm both MSI legs go green.

## Superseded (2026-08-11): Task Installer.1.2 — NSIS retired, arm64 ships an `.msi`

> The "arm64 still ships two of three apps" statement below was overtaken by
> commit `fba962c4d8` — arm64 now ships all three apps, built natively.

**Branch `task-installer-win-arm64-msi`, off `run-seamlyLayout`.**

Windows now ships **MSIs only**. `ci.yml`'s `windows` job — the last NSIS
producer, arm64-only since Installer.1.1 — is deleted, and `windows-msi` became
a **matrix over `arch`** (`x64`, `arm64`, `fail-fast: false`), which is
`windows-msi.yml`'s `msi` job verbatim minus its own version step. `publish`
releases `seamly-x64.msi` + `seamly-arm64.msi` and no longer needs the `windows`
job.

**Why NSIS could go at all:** the stated reason for keeping it (no arm64
SeamlyLayout build) never justified NSIS. `windows-msi.yml` has always built an
arm64 MSI with `smsi.ps1 -NoSeamlyLayout`, carrying seamly2d + seamlyme —
*exactly* the two apps the arm64 NSIS package carried. The format swap loses
nothing; shipping SeamlyLayout on arm64 is a separate, still-open problem.

**arm64 still ships two of three apps.** SeamlyLayout needs an
`aarch64-pc-windows-msvc` Rust + cxx-qt build and an arm64 Qt WebEngine (for
`SvgCanvas.qml`), neither of which exists. Tracked in the newly written
`TODO_INSTALLER_WIN_ARM64.md` as InstWinArm64.1, with the exact three-line
change to both workflows when it lands.

**`dist/seamly2d-installer.nsi` was deleted** (Task InstWinX64.11.1). It was
first kept unbuilt as the record of what a pre-MSI installation left on disk.
Task InstWinX64.11.2 transcribed that footprint into `smsi.wxs`, above the
`RemoveLegacyProgramFiles` component, so the MSI's removal authoring keeps its
record and the file could go.

`-WinDeployQt6` was deliberately not passed on the arm64 leg while that leg
shipped two apps. **Superseded twice since:** both legs pass the deploy tool,
and on 2026-08-15 the parameter was renamed `-WinDeployQt` and `$includeLayout`
was removed with `-NoSeamlyLayout`.

Docs updated: `README-BUILDS.md`, `README_WORKFLOWS.md`, `TODO_INSTALLER.md`
(Installer.1.2 checked off), `TODO_INSTALLER_WIN_ARM64.md` (written from a stub),
`TODO_MIGRATE.md` M.12 arm64 row, `TODO_CODE_SIGNING.md` CodeSign.1.7 (closed as
moot — it was explicitly conditional on NSIS still being the released installer).

**Verification is the CI run for this commit** — there is no YAML tooling on this
PC and no arm64 hardware. Confirm both MSI legs go green and that
`seamly-arm64.msi` uploads.

## Also 2026-08-11: CI version numbers could be octal C++ literals — fixed

**Branch `task-ci-version-octal`, off `run-seamlyLayout`.**

Run [`31447296387`](https://github.com/seamly/Seamly2D/actions/runs/31447296387)
failed **all four build jobs** (macOS, Linux AppImage, Windows NSIS arm64,
Windows MSI x64) on the same error:

```text
projectversion.cpp:67:43: error: invalid digit '8' in octal constant
   67 | extern const int SUPER_MINOR__VERSION = 048;
```

Not a code regression — a **time-of-day bug**. `ci.yml:35` and
`windows-msi.yml:99` build the version with `date +%Y.%-m.%-d.%-H%M`; the run
started at 00:48 UTC, so the fourth component was `048`, and `scripts/version.sh`
substituted it verbatim into `projectversion.cpp` as a C++ integer literal. A
leading zero makes that literal **octal**: it fails to compile when the minutes
contain an 8 or a 9, and — more quietly — compiles to the *wrong number*
otherwise (`047` is 39). Any build started in the 00:00 UTC hour was affected.

**Fix is in `scripts/version.sh`, not the workflows**, so it covers both YAML
call sites at once: every component is validated as numeric and normalized with
`$((10#…))`, and `VERSIONSTR` is rebuilt from the normalized parts so
`VER_FILEVERSION_STR` and the two `Info.plist` files agree with the integers. A
non-4-part or non-numeric argument is now rejected instead of corrupting the
source files.

`scripts/version_test.sh` is new: it drives `version.sh` over the release
case, the `048` regression, the silently-wrong `047` case and an all-zero
component, asserts no `= 0[0-9]` literal is ever written, checks both rejection
paths, and backs up/restores all four files it rewrites. **26/26 assertions
pass.** No local qmake build was run — no C++ changed, and the emitted
`projectversion.cpp` is restored byte-for-byte by the test.

## Verification happens on GitHub CI, not on this PC

User's instruction, 2026-08-10: concentrate on the CI pre-releases and ignore
local MSVC/qmake builds. That also settles what to do about the broken Visual
Studio install below — nothing, for now.

A plain push run **skips `publish`** — only a `schedule` or `workflow_dispatch`
run on `run-seamlyLayout` exercises the pre-release path, so dispatch one to
prove the release step end to end.

**`gh` is installed and authenticated** (2026-08-11), at
`C:\Program Files\GitHub CLI\gh.exe`. Read run state with it directly —
`gh run list --repo seamly/Seamly2D --branch run-seamlyLayout --workflow ci.yml`,
then `gh run view <id>` and `gh run view --job <id> --log-failed`.

## Follow-up left open deliberately

`.github/README.md`'s "Windows 64-bit" download badge still points at upstream's
`Seamly2D-windows.zip`. It has to become `seamly-x64.msi`, but only when the
migration is pushed upstream — the badges link to
`FashionFreedom/Seamly2D/releases/latest`, so editing them now breaks the live
public download link. Tracked as **Task M.12** in `TODO_MIGRATE.md`.

## MACHINE STATE: the Visual Studio installation is broken — and that is now accepted

**The user's decision (2026-08-10): build and verify on GitHub CI; do not try to
build locally with MSVC.** So this section is background, not a blocker. Do not
spend a session repairing Visual Studio unless the user asks.

The local build scripts were deleted on 2026-08-15, so nothing in the repository
depends on this any more. Keep the findings — a hand-built local tree hits the
same wall. The symptom was `'cl' is not recognized`. Not the script, and not the
agent sandbox — the same failure occurs with the sandbox disabled:

- `vcvars64.bat` **and** `vcvarsall.bat x64` exit 1 with
  `[ERROR:VsDevCmd.bat] *** VsDevCmd.bat encountered errors ***`; three
  sub-scripts fail to init — `core\msbuild.bat`, `ext\cmake.bat`,
  `ext\ConnectionManagerExe.bat`
- The toolset is fine on disk: `cl.exe` 19.51.36252 under
  `VC\Tools\MSVC\14.51.36231`, Windows SDKs 10.0.22621.0 / 10.0.26100.0 present
- **Plain `vswhere -products *` returns nothing**; only
  `vswhere -all -prerelease -legacy` finds
  `C:\Program Files\Microsoft Visual Studio\18\Community`. This is *"Visual
  Studio 2026 Developer Command Prompt v18.8.1"*, a prerelease build, and the
  instance registration looks damaged. Instance data exists at
  `C:\ProgramData\Microsoft\VisualStudio\Packages\_Instances\a9afd7ad`

**Workaround — local only, nothing on the machine was changed:** set
`PATH`/`INCLUDE`/`LIB` by hand at `VC\Tools\MSVC\14.51.36231` + SDK
`10.0.26100.0`, then run `C:\Qt\Tools\QtCreator\bin\jom\jom.exe -f Makefile` in
the shadow-build directory. **The user should repair VS 18 Community from the
Visual Studio Installer** before attempting any local MSVC build.

## Other machine state changed outside the repo

`WixToolset.Util.wixext` 6.0.2 is installed globally
(`wix extension add --global`), because `util:RemoveFolderEx` needs it.
Reversible with `wix extension remove`. `smsi.ps1` requires it, and both
`.github/workflows/windows-msi.yml` and `ci.yml`'s `windows-msi` job install it
alongside the UI extension.

## SAFETY — read before deleting anything

- **`C:\Users\susan\seamly2d` on the test laptop is live user data.** It holds a
  real pattern of the user's and is the source tree Task 60's migration copies
  from. A `Remove-Item -Recurse` on it was suggested in an earlier session and
  withdrawn: it bypasses the Recycle Bin, and that is how the dev PC's copy was
  permanently destroyed.
- **No test may touch a path under `QDir::homePath()`** — it cannot be faked on
  Windows. First-run resolution is tested through
  `VCommonSettings::chooseFirstRunDataRoot()`, which takes both candidate roots
  as arguments; everything else uses `QTemporaryDir`.
- **`CollectionTest` is deliberately not run locally.** It has a documented
  pre-existing failure *and* launches the real seamly2d, which seeds folders into
  the live data root. CI runs it on a clean machine.

## Decisions the user has ANSWERED

Act on these, do not re-ask, remove from this file for the next Session handover if marked as done or added to a TODO_*.md file

1. **Task 54's file-name form** → **`SettingsCommon.h`**, i.e. the file name
   matches the class name (the style guide's class-match exception wins over the
   `settings_*` snake_case prefix for class-defining files). **Added another rename to TODO_RENAME_SETTINGS_FILES_CLASSES.md**
2. **`.github/README-DEVELOPER-NEW.md`** → **rename it to
   `.github/README-DEVELOPER-SEAMLY-APPS.md`**, to be folded into
   `.github/README-DEVELOPER.md` when the migration is complete. **DONE.**
3. **Qt WebChannel / Qt Positioning documentation** → maintain it in
   `.github/README-DEVELOPER-SEAMLY-APPS.md` until the migration completes.
4. **`src/app/seamly2d/core/BUILD_PROBLEMS.txt`** → delete it if it is not
   useful. **Done**
5. **Testing happens on the test laptop, not in a VM** (re-confirmed 2026-07-30).
   This PC is Windows 11 **Home**, which ships neither Hyper-V nor Windows
   Sandbox, so a VM here means a third-party hypervisor; the user considered
   VirtualBox and VMware Workstation Pro and declined both. A VM could not close
   two checklist items anyway — the *verified-publisher* UAC prompt needs Task
   33's signing, and the arm64 repeat needs arm64 hardware. **ADDED to TODO_INSTALLER.md**
6. **Data migration is copy-and-verify, leaving the legacy tree intact** — never
   a bare rename, because a user may need to roll back to an earlier release. **ADDED to TODO_INSTALLER.md**
7. **The migration lives in the applications, not the installer.** A per-machine
   MSI's server side runs as LocalSystem, so a per-user path resolves to the
   SYSTEM profile and would only cover whoever ran setup; macOS and Linux have no
   MSI at all, so the logic would be written twice. **ADDED to TODO_INSTALLER.md**
8. **The program directory in Windows is `C:\Program Files\` + `Seamly`** — show the user
   the final assembled path and take OK/Cancel, rather than editing a box whose
   contents differ from the path it applies. **ADDED to TODO_INSTALLER_WIN_X64.md**
9. **Data-root relocation asks first** — prompt Y/N before copying existing data
   files to a new directory location. **ADDED to TODO_INSTALLER.md**
10. **The MSI steps are inlined in `ci.yml`**, not factored out of
    `windows-msi.yml` as a reusable `workflow_call`. The two copies of the x64
    build steps must therefore be kept in step by hand. **ADDED to TODO_INSTALLER_WIN_X64.md**
11. **The x64 `.msi` replaces the NSIS Windows zip** rather than shipping
    alongside it. NSIS stays for arm64 until Task Installer.1.2, because there is
    still no arm64 SeamlyLayout build. **ADDED to TODO_INSTALLER_WIN_X64.md**
12. **Pre-releases are cut from `run-seamlyLayout`**; `develop` stays a pristine
    upstream mirror.** Nothing is published from `develop` until the whole
    SeamlyLayout migration is finished and pushed upstream in one go —
    incremental upstream commits are not workable given the size of the change.
    **ADDED to TODO_INSTALLER.md**

## Gotchas

- **`CollectionTest.exe` must be run with its working directory set to its own
  `bin/`.** `initTestCase()` removes `tst_seamly2d_tmp` *relative to the CWD* but
  re-creates it under `applicationDirPath()`, so from any other CWD it aborts on
  the leftover directory from the previous run ("Fail to prepare test files for
  testing"). Use `Start-Process … -WorkingDirectory <that bin>`.
- **A SeamlyLayout build-tree exe needs Qt on `PATH` to launch.** There is no
  windeployqt output beside `qt_frontend/build/Debug/SeamlyLayout.exe`, so from a
  plain shell it starts and does nothing — no log file is even created. Prepend
  `C:\Qt\6.11.1\msvc2022_64\bin`. `ctest` handles this itself via the
  `ENVIRONMENT_MODIFICATION` added in Task 58. **Not a problem when building with ci.yml on GitHub with Ubuntu-latest runner**
- **SeamlyLayout's log file has two independent writers and they overwrite each
  other.** C++ `Logger` holds a buffered `QTextStream` on the file while Rust's
  `log_to_file()` opens/appends per call, so lines get clipped mid-string. Do not
  conclude a log line is absent because it looks truncated — grep for a
  distinctive fragment. **Added this as a task in TODO_SEAMLYLAYOUT.md**
- **A tagged handoff SVG can be produced headlessly**, without driving the Layout
  Mode GUI: `seamly2d.exe <pattern>.sm2d -b <name> -d <dir> -f 0 --exportOnlyDetails`
  writes `<name>_pieces.svg` through the same `exportSVG()` that
  `generatePiecesSvg()` uses.
- **`QCommandLineParser::parse()` ≠ `process()`.** `process()` prints to a console
  this GUI-subsystem app does not have on Windows, and calls `exit()`. `parse()`
  returns a bool and fills `errorText()`.
- **A `develop` merge can silently drop doc edits made on this branch.** After
  merging `develop`, `git log -S "<a phrase you added>" -- <file>` is the cheapest
  way to confirm your change survived.
- **`QSettings(fileName, format, parent)` records neither an organization nor an
  application name** — both come back empty, and QSettings substitutes the literal
  `"Unknown Organization"`. Root cause of the stray files in Tasks 34 and 52.
  `QSettings::setPath(format, scope, dir)` *does* redirect settings files, but has
  **no getter** — recover the base from a probe instance.
- **`QDir::fromNativeSeparators()` rewrites backslashes only on Windows** (a
  backslash is a legal POSIX filename character), and Windows path comparison must
  be `Qt::CaseInsensitive`.
- **`QDir::rmdir()` over `removeRecursively()`, deliberately.** `rmdir()` cannot
  delete a file and refuses a non-empty directory, so it cannot run away.
- **CI's `make check` runs four test binaries** — `Seamly2DTest`,
  `CollectionTest`, `ParserTest`, `TranslationsTest`. Any hand-run local check
  must cover all four; `Seamly2DTests.exe` alone is one of them.
- **`gh` is on `PATH` and authenticated** as of 2026-08-11; plain `gh …` works.
  If a shell ever comes up without it, invoke
  `& "C:\Program Files\GitHub CLI\gh.exe"`.
- **A CI version component with a leading zero is a C++ octal literal.**
  `scripts/version.sh` now normalizes them; do not "simplify" that `$((10#…))`
  away. Covered by `scripts/version_test.sh`.
- **There is no YAML tooling on this PC.** No `actionlint`, no `yamllint`, no
  `node`, and `python`/`python3` are the Microsoft Store stubs. Git's `perl` has
  no YAML module. Workflow edits can only be validated by pushing and watching
  the run.
- **The sandbox blocks a command containing both a `Remove-Item` and a protected
  path string** — `G:` paths and `C:\Program Files` have both triggered it, even
  when the deletion targets something else entirely (the MSVC environment
  variables set at the top of a build command are enough). Put deletions in
  their own call.
- **Renaming a shadow-build directory invalidates the whole tree.** qmake bakes
  absolute paths into every generated Makefile, so after a rename the sub-builds
  still look for `...\<old name>\...\vtools.lib` and fail with "dependent ...
  does not exist". The tree can only be regenerated, not repaired: delete it and
  re-run qmake. Same failure shape as the toolchain-change trap in
  `.github/README-BUILDS.md`. The build and packaging directory names are now
  `-BuildDirName` / `-OutputDirName` parameters; **a new value needs a matching
  `.gitignore` entry**, and for packaging also the CI artifact path.
- **PowerShell here-strings passed to `git commit -m` get mangled** when the
  message contains quotes; write the message to a file and use `git commit -F`.
- **clangd diagnostics in this repo are noise.** The tree has no
  `compile_commands.json`, so the editor parses each file with zero include
  paths; one unresolved include cascades into dozens of `Unknown type name 'QString'`
  entries. **The qmake build is the authority.**
- **`MD060`/`MD056` table warnings from the editor are noise** on the wide spec
  tables in `TODO_MIGRATE.md`, as `MD041` is repo-wide. Fix a genuinely malformed
  row; do not reflow tables to silence alignment style.
- **PowerShell 5.1 wraps a native exe's stderr in `NativeCommandError`** and sets
  `$?` to `$false` even on exit 0. Do not redirect native stderr inside
  PowerShell — run the script as a child process with
  `Start-Process … -RedirectStandardOutput/-RedirectStandardError -Wait -PassThru -NoNewWindow`.
- **PowerShell splatting: `@array` is positional, `@hashtable` is by name.**
- **Qt frontend test exes are GUI-subsystem binaries** — they print nothing to
  captured stdout. Run with `-o <file>,txt` and `QT_QPA_PLATFORM=offscreen`.
- **`$proFile` collides with the automatic `$PROFILE`** (case-insensitive) in
  PowerShell. Do not use that variable name in a new script.
- **Historical 6.10 references and old directory names in
  `project-docs/TODO_COMPLETED.md` and `project-docs/PROJECT_PLAN.md` are
  deliberate** — they record what was true at the time.
