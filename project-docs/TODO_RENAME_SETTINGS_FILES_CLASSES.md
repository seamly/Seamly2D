# TODO — Create the combined MSI installer for Seamly2D, SeamlyMe, and SeamlyLayout

Tasks for creating an .msi file for installation on a user's amd64 computer with Windows 10 or Windows 11.

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

## Task 14 — Windows installer: choose program and user-data paths

**Dependencies:** Task 13 (family MSI), Task 34 (`SeamlyData` rename), Task 38 (standalone-install replacement and data protection).

- [ ] **14.1 Program directory**

  - Default to `C:\Program Files\SeamlyApps`.
  - Update `seamly-family.wxs` and all shortcuts, file associations, registry values, and SeamlyLayout paths.
  - Accept any local or removable drive/path; reject cloud-synced locations.
  - Add a `Change` button and silent-install property.

- [ ] **14.2 User-data directory**

  - Default to `C:\Users\<user>\SeamlyData`.
  - Accept any drive/path, including OneDrive, Google Drive, Dropbox, external drives, and USB media.
  - Add a `Change` button and silent-install property.

- [ ] **14.3 Persist both selections** and prefill them during repair or upgrade.
- [ ] **14.4 Register the data root** through a dedicated environment variable, registry value, or application setting so all three apps use it without prompting. Document the selected mechanism.
- [ ] **14.5 Add the executable and data directories to the current user’s `PATH`**, broadcast the change, and remove only installer-created entries during uninstall.
- [ ] **14.6 When the data root changes:**

  - Explain that first run will copy existing data without deleting the source.
  - Require confirmation before continuing.
  - Copy only missing files; never overwrite existing destination files.

- [ ] **14.7 Make Seamly2D, SeamlyMe, and SeamlyLayout honor the configured data root on first run and thereafter.**
- [ ] **14.8 Preserve user data during uninstall;** remove applications, shortcuts, registry/configuration entries, and installer-created `PATH` entries only.
- [ ] **14.9 Optionally offer to launch the Seamly applications after installation.**
- [ ] **14.10 Document** both prompts, silent-install properties, path registration, migration behavior, and uninstall behavior in:

  - `scripts/packaging/windows/README.md`
  - `.github/README-BUILDS.md`

### Verification

- [ ] **14.11 Fresh install**

  - Programs: `C:\Program Files\SeamlyApps`
  - Data: `C:\Users\<user>\SeamlyData`

- [ ] **14.12 Standalone migration**

  - Programs: `C:\Program Files (x86)\Seamly2D` → `E:\Programs\SeamlyApps`
  - Data: `C:\Users\<user>\seamly2d` → `E:\SeamlyData`

- [ ] **14.13 Cloud-data migration**

  - Programs: `C:\Program Files\SeamlyApps`
  - Data: `G:\My Drive\seamly2d` → `G:\My Drive\SeamlyData`

- [ ] **14.14 For each scenario, verify:**

  - The installer remembers and registers both paths.
  - Desktop shortcuts are created.
  - `.sm2d`, measurement, and SVG files open in the correct app.
  - Existing data is copied without overwrite or deletion.
  - Upgrade preserves both paths.
  - Uninstall removes apps, shortcuts, and installer-created `PATH` entries while preserving data.

## Task 13 — Windows MSI installer (x64 and ARM64)

Build one WiX v6 MSI per architecture containing Seamly2D, SeamlyMe, SeamlyLayout, and their dependencies. Store application state in the appropriate AppData directories; user-data paths and migration are covered by Task 14.

**Related:** Task 14 (path selection and data migration), Task 34 (data-root structure), Task 38 (standalone-install replacement).

- [ ] **13.1 Configure WiX v6** using `scripts/packaging/windows/seamly-family.wxs`. Do not adopt WiX v7 until its OSMF EULA is approved.
- [ ] **13.2 Build x64 and ARM64 MSIs** containing:

  - Seamly2D, SeamlyMe, and SeamlyLayout
  - Qt 6.11.1 runtime, QML modules, and WebEngine
  - App-local MSVC runtime
  - Statically linked Rust dependencies
- [ ] **13.3 Add Windows integration:**

  - Start Menu shortcuts for all three apps
  - `.sm2d`, `.smis`, and `.smms` associations
  - In-place major upgrades
  - Clean application uninstall
- [ ] **13.4 Preserve all user data** during installation, upgrade, repair, and uninstall.
- [ ] **13.5 Sign MSI artifacts with `jsign`** when `SEAMLY_SIGNING_PROJECT_ID` is available.
- [ ] **13.6 Support builds through:**

  - Local: `scripts/smsi.ps1`
  - CI: `.github/workflows/windows-msi.yml`
- [ ] **13.7 Test both architectures**, where hardware is available:

  - Clean installation and launch
  - Shortcuts and file associations
  - Upgrade and repair
  - Uninstall without data removal

- [x] **13.8 Complete static x64 validation** — passed July 22, 2026; clean-machine testing remains.
- [ ] **13.9 Document building, signing, and verification in:**

  - `scripts/packaging/windows/README.md`
  - `.github/README-BUILDS.md`
  - `.github/workflows/README_WORKFLOWS.md`

## Task 32 — Suppress `.wixpdb` generation

The WiX v6 MSI build needs only the `.msi`; suppress the optional linker database with `wix build -pdbtype none`.

- [x] 32.1 Add `-pdbtype none` to `$wixArguments` in `scripts/packaging/windows/smsi.ps1`, covering x64, ARM64, local, and CI builds.
- [ ] 32.2 Run a full Seamly MSI build and confirm:

  - [ ] 32.2.1 No `.wixpdb` is generated.
  - [ ] 32.2.2 `wix msi validate` passes.
  - [ ] 32.2.3 Windows Installer COM inspection passes.
  - [ ] 32.2.4 Pending restoration of the Qt build environment tracked in Task 31.

- [x] 32.3 Confirm no scripts, workflows, or inspection tools require `.wixpdb`.
- [x] 32.4 Document that `.wixpdb` is suppressed by default and can be restored by removing `-pdbtype none` from `$wixArguments`.

# TODO — Build the Seamly2D, SeamlyMe, and SeamlyLayout executables on this local PC

## Task 46 — Prevent stale qmake Makefiles after Qt changes

After a Qt change, `sd.ps1` may regenerate the top-level Makefile while reusing stale sub-Makefiles. This can produce misleading missing-library errors. Deleting the shadow-build directory resolves the problem.

- [ ] 46.1 Add `scripts/sd.ps1 -Clean` to remove `scripts/seamly2d-debug/` before configuration.
- [ ] 46.2 Detect qmake/Qt kit changes automatically and recreate the debug build tree before configuring.
- [ ] 46.3 Apply equivalent protection to the release `build/` directory.
- [ ] 46.4 Check `scripts/st.ps1` for the same stale-Makefile behavior and fix it if affected.
- [ ] 46.5 Document automatic cleanup and `-Clean` usage in:

  - [ ] 46.5.1 `sd.ps1` help
  - [ ] 46.5.2 `.github/README-BUILDS.md`

## Task 51 — Windows MSI: finish the install-time experience (shortcuts, registry, ARP, associations, UAC, upgrade warning)

Task 30's verification exercised the MSI's **payload** — the flat install directory, the single shared Qt 6.11.1 runtime, and all three apps launching from it — by extracting the package with `msiexec /a` and running the exes out of the expanded tree. That deliberately stops short of everything Windows Installer does *around* the files. This task covers that remainder, and adds the install-time choices and warnings the installer should be offering the user but does not yet.

Related: **Task 13** authored the shortcuts/associations/upgrade behaviour and its last subtask still wants a clean-machine install/uninstall/upgrade cycle; **Task 14** covers the install-path prompts; **Task 34** is the `~/seamly2d` → `~/seamlyData` data-root rename this task's upgrade warning refers to. Keep the wording here consistent with whatever Task 34 lands.

**Progress 2026-07-28.** Everything that can be settled without an elevated install on a clean machine is done: the authoring work (desktop-shortcut checkbox, previous-installation warning, NSIS detection) plus a new `scripts/packaging/windows/test_msi_authoring.ps1` that opens the built MSI with the Windows Installer COM API and asserts ~50 expectations about its contents. `smsi.ps1` runs it on every build, so CI runs it for both architectures. The four "verify …" subtasks below are therefore *half* done — the package provably contains the right rows, but whether a shortcut launches or Explorer shows the right icon can only be seen on a real machine, so they stay open and are closed by the clean-machine cycle at the bottom. The user's decision (2026-07-28) was **not** to install on the developer PC.

**Progress 2026-07-29.** The install cycle is now *executable* rather than a prose checklist. New `scripts/packaging/windows/test_msi_install.ps1` verifies a real install in four phases (`Baseline` / `Installed` / `Upgraded` / `Removed`) run around the `msiexec` commands, sharing a state file so the uninstall phase can prove that the user's data survived by comparing inventories rather than by assertion. It is standalone — no repository, build tree or Qt on the test machine — and it covers the one thing package inspection can never reach: **it starts each app and checks it stays running**, which is the only way a missing Qt DLL or QML module shows up. Self-tested on the developer PC: the `Baseline` phase passes and correctly reports the machine's NSIS install and its three user-data trees, and a deliberate negative run of `Installed` against a machine with nothing installed fails with exit 1, so the checks are known not to be vacuous. Two packages were built for the upgrade leg (project versions **2026.7.28.2355** and **2026.7.29.0041**, same `UpgradeCode`, different `ProductCode`) and staged with the checker and a sample pattern in `scripts/seamly-msi/task51-test-kit/` (gitignored) alongside a `RUN-ME-FIRST.md` walkthrough. **The user's decision (2026-07-29) is to run the cycle on the test Windows 11 laptop, not on the developer PC**, so the five subtasks below stay open until that transcript comes back.

**Progress 2026-07-30 — the cycle was run on the test laptop, and it found three real defects.** The `Baseline`, install and `Installed` phases were run against `Seamly-x64-older.msi`, then the upgrade to `-newer.msi` with `/l*v` logging. **52 of 57 automated checks passed**, including the ones that matter most: all three apps start and stay running (so the deployed Qt/WebEngine runtime is complete), all three file associations resolve and a real `.sm2d` opens through ShellExecute, the desktop shortcuts and their registry breadcrumbs are correct, and the ARP entry carries the right name, publisher, version, comments, links, size and uninstall string. Exactly one UAC prompt. `SeamlyPreviousInstallDlg` displayed correctly (log: `Action start 18:20:16` → `Dialog created` → `Return value 1`), in the right position — before Welcome → EULA → Destination Folder → Ready.

Three defects came out of it, none of which static verification could have found:

1. **`SeamlyShortcutsDlg` never appears.** Not a packaging error — the `ControlEvent` row is in the shipped MSI, correct in every column (`InstallDirDlg` / `Next` / `SpawnDialog` / `SeamlyShortcutsDlg`, condition `1`, ordering 2, ahead of the built-in `NewDialog` at 4), and the `Dialog` row has `Attributes = 7` (Visible + Modal + Minimize). The verbose log shows **no attempt to create it**, and a failed creation would have logged 2803/2826 as the other dialogs do. The design notes were written against the WiX v3/v4 `InstallDirDlg`, which publishes `DoAction WixUIValidatePath` + a conditional `SpawnDialog InvalidDirDlg`; **this is WiX 6.0.2**, whose `InstallDirDlg` publishes `CheckTargetPath` — a v6 built-in implemented in the UI extension's `uica.dll`. Our `SpawnDialog` is skipped somewhere in that new chain. Because `SEAMLYDESKTOPSHORTCUTS` defaults to 1, the shortcuts were created anyway and every automated check around them passed: the default works, the *choice* is never offered. **Still open — see the new subtask below.**
2. **Dialog geometry.** `SeamlyPreviousInstallDlg`'s `BannerLine` and `BottomLine` are `Width="373"` on a 370-wide dialog, which raises error 2826 twice. The stock WixUI dialogs log the same code at `DEBUG:` level only; ours is additionally logged as a user-facing "The installer has encountered an unexpected error … 2826". Not observed on screen, but it is three characters to fix. **Still open — see the new subtask below.**
3. **The user-data tree is never seeded, whichever root is chosen — FIXED here.** `VCommonSettings::ensureDataRootTree()` creates the nine standard subfolders, but its only production caller was `setDataRoot()`, which runs only when the user *changes* the root in Preferences → Paths. First run goes through `initializeDataRoot()`, which resolves the path and writes the setting directly, so nothing ever seeded the tree: on a genuinely fresh machine the root is recorded as `~/seamlyData` and never created, and on an upgrading machine the adopted tree never gains the subfolders it lacks. The `pruneEmptyLegacyDataRoot()` doc comment already asserted "ensureDataRootTree() will have stocked it with the nine standard subfolders", which was simply not true. **Note what this defect is *not*:** on the test laptop `~/seamlyData` was correctly absent and the data root was `C:\Users\susan\seamly2d`, because the old NSIS build had left that directory and `chooseFirstRunDataRoot()` **adopts** an existing legacy tree in place rather than moving gigabytes of patterns — intended Task 34 behaviour, not a bug. Fixed by calling `ensureDataRootTree(dataRoot())` from `Application2D::openSettings()` and `ApplicationME::openSettings()` — in the applications rather than inside `initializeDataRoot()`, because that is the only place the real home directory reaches it and the unit tests do call `initializeDataRoot()` (the Task 34/53 rule). New regression test `TST_DataRoot::StartupResolvesThenSeedsTheConfiguredRoot` pins both halves: that resolution stays free of disk side effects, and that seeding then produces all nine folders.

**A bug in the checker itself was found and fixed by the same run.** All three Start Menu shortcuts reported `FAILED … points into the install directory - target = 'C:\Windows\Installer\{ProductCode}\seamly2d.ico'`. They are **advertised** shortcuts — `seamly-family.wxs` nests each inside its `<File KeyPath="yes">` with no `Target`, WiX's standard pattern — and `WScript.Shell` does not report an advertised shortcut's target; it hands back the extracted icon path. The script assumed an unresolvable advertised shortcut would come back *empty*, so nothing ever reached that branch and three correct shortcuts failed every run. `test_msi_install.ps1` now resolves the Darwin descriptor properly through `MsiGetShortcutTarget` + `MsiGetComponentPath`, which asserts something stronger than before: that the shortcut resolves to an installed file *inside this install directory*.

**Progress 2026-08-02 — run 2 on the test laptop.** Packages `26.7.44158` → `26.7.44161` from current source; transcript and verbose log kept in `scripts/seamly-msi/task51-test-kit/` as `task51-run2.txt` and `task51-upgrade.log`. The test machine is **Windows 10 22H2 (10.0.19045), PowerShell 5.1** — not Windows 11, which is worth knowing because the "Apps & features" finding below is a Windows-10 Settings behaviour. The `Removed` phase was **not run**: the tester issued `msiexec /x` and then `Stop-Transcript`, so every "…and is removed on uninstall" half of the subtasks below is still unproven and the cycle needs one more short leg.

Three things this run set out to prove, all three proven:

1. **Programs install to `C:\Program Files\SeamlyApps`** — registry `InstallPath` read back as `C:\Program Files\SeamlyApps\`, and unchanged across the upgrade.
2. **The NSIS installation is removed by the install (step 2a)** — all five checks passed: install directory gone, `Install_Dir` key gone, ARP entry gone, Start Menu folder gone, and the MSI provably installed somewhere else. This **answers the second open question above** — run 1's disappearance was the tester, run 2's is the package. `RemoveNsisProgramFiles` + `RemoveNsisRegistryKeys` work, with still no `CustomAction` in the package.
3. **Task 60's migration works end to end on a real profile** — `Documents\Seamly` was created with all eight of the legacy tree's folders *plus* `images`, including the user-added `bodyscans`, the four files came across, the legacy tree was left intact (4 → 5 files, the gain being the marker), and `MIGRATED-TO-SEAMLY.txt` names the new root and the date. Wholesale copy, nothing stranded, nothing deleted.

Also passing: exactly one ARP entry after the upgrade, a newer build, an unmoved install directory, all three apps starting and staying running, all three associations resolving, and a real `.sm2d` opening through ShellExecute.

**Four checks failed in both phases, and neither is a package defect.**

- **The three Start Menu shortcut failures are a second checker bug**, the same shape as run 1's. `Get-AdvertisedShortcutTarget` treats `MsiGetComponentPath` states **4/5** as the ones yielding a usable path ([test_msi_install.ps1:455-457](scripts/packaging/windows/test_msi_install.ps1#L455-L457)); the actual constants are `INSTALLSTATE_LOCAL = 3` and `INSTALLSTATE_SOURCE = 4`. All three shortcuts returned **3**, i.e. installed locally — a pass reported as a failure. The fix is `-eq 3 -or -eq 4`. The shortcuts have now been correct in both runs and wrong in the checker both times.
- **ARP `DisplayIcon` — the first open question above is now answered.** The verbose log shows `ARPPRODUCTICON = seamly2d.ico` in the Property table and `ProductInfo(… ProductIcon=seamly2d.ico …)` actually executing, so the authoring is right: Windows Installer records the icon as *product metadata* and does not write `DisplayIcon` into the Uninstall key. The tester's by-eye check corroborates it exactly — legacy `appwiz.cpl` ("Uninstall or change a program") shows the icon and the publisher, while the Settings "Apps & features" page shows neither, because Settings reads the registry value that MSI never wrote. Fix: author `DisplayIcon` as a registry value, or assert it through `MsiGetProductInfo` instead of the registry.

**Two checker weaknesses worth fixing before the next run.** The `Upgraded` phase reported `Documents\Seamly` "did not exist at `Installed` — nothing to preserve", because the `Installed` snapshot is taken before the app has ever run and therefore before the migration; the migration target is never compared across the upgrade, which is precisely the comparison worth having. And `sample-pattern.sm2d` depends on a measurement file `./2025-06-08-Sue.smis` that the kit does not carry, so the association test proves seamly2d *opens* but leaves it sitting on a "locate the measurement file" prompt — the pattern in the kit should be self-contained.

**The tester's UI findings are a branding pass, filed as their own subtask below.** The wizard says "Seamly2D" throughout where it now installs three applications, and the previous-install dialog's data paragraph still names `seamlyData`.

**Run 2's findings are filed as Tasks 61-67 below** rather than as subtasks here, because they are independent fixes across three different areas — the checker, the package authoring and the applications — and Task 51 is already long. This task keeps only the verification gates.

- [ ] **Run the uninstall leg** — run 2 stopped after `msiexec /x` without `-Phase Removed`, so nothing about removal has been verified on a real machine yet. This is the last gate on the four "verify …" subtasks below and on Task 13's outstanding subtask. Do it after **Task 61**, or the same four false failures will reappear in the transcript
- [ ] **Fix `SeamlyShortcutsDlg` never displaying** (found 2026-07-30, above). The `SpawnDialog` published on `InstallDirDlg`'s Next is skipped by WiX 6.0.2's `CheckTargetPath` chain. Iterate against a small UI-only MSI carrying the same `ui:WixUI` reference and the same dialogs — it builds in seconds and can be clicked through and cancelled at the Ready page without installing anything — rather than rebuilding the 165 MB package per attempt. Candidate approaches: a `NewDialog` to `SeamlyShortcutsDlg` at an ordering below the built-in one (making it a real wizard page, with its own Back/Next), or scheduling it in `InstallUISequence` the way `SeamlyPreviousInstallDlg` demonstrably works. Whatever lands must be asserted by `test_msi_authoring.ps1` **and** confirmed by a real wizard run, since authoring passed while the page never appeared
- [ ] **Fix the dialog geometry** (found 2026-07-30, above): `SeamlyPreviousInstallDlg`'s `BannerLine`/`BottomLine` are `Width="373"` on a `Width="370"` dialog, raising error 2826 twice per install
- [ ] **Verify Start Menu shortcuts** on a real install: all three advertised shortcuts (`seamly2d`, `seamlyme`, `SeamlyLayout`) appear, point at the installed exes, carry the right icons and `WorkingDirectory`, launch each app, and are removed cleanly on uninstall — *authoring verified*: all three rows are in `ProgramMenuFolder`, advertised (`Target` = the feature), each with an `Icon_` and `WkDir` = `INSTALLFOLDER`; asserted by `test_msi_authoring.ps1`. Runtime behaviour still needs the clean machine, and is now scripted — `test_msi_install.ps1` checks each `.lnk` exists in the All Users Start Menu, resolves its target where Windows will resolve an advertised shortcut, starts each app to prove the runtime is complete, and asserts all three are gone after uninstall
- [ ] **Verify the registry rows** written by `seamly-family.wxs` — `HKLM\SOFTWARE\Seamly\Seamly2D` `InstallPath` (and any sibling values) exist after install, hold the actual install directory, and are removed on uninstall — *authoring verified*: `InstallPath` and `DisplayVersion` (full `YYYY.M.D.HHMM`) are present, plus the new `DesktopShortcutSeamly2D`/`DesktopShortcutSeamlyMe` breadcrumbs; component rules remove them on uninstall. `test_msi_install.ps1` reads the key back on a real install — that `InstallPath` names a directory that exists, that `DisplayVersion` matches the `YYYY.M.D.HHMM` shape, that the breadcrumbs track the checkbox — and asserts the whole key is gone afterwards
- [ ] **Verify the Add/Remove Programs entry** — the product appears in Apps & features / ARP with the correct DisplayName, DisplayVersion (the `26.7.x` MSI ProductVersion *and* the full project version as `DisplayVersion`), Publisher, icon (`ARPPRODUCTICON`), help/about links (`ARPHELPLINK`, `ARPURLINFOABOUT`), an accurate estimated size, and that Uninstall from there removes the product completely — *finding*: **ARP's DisplayVersion cannot carry the project version.** `RegisterProduct` writes the Uninstall key after `WriteRegistryValues`, so any component-authored override is overwritten; ARP therefore shows `26.y.z` and the full version is surfaced through the new `ARPCOMMENTS` property and `HKLM\SOFTWARE\Seamly\Seamly2D`. Estimated size is computed by Windows Installer itself from the installed files — nothing to author, and now checked rather than eyeballed: `test_msi_install.ps1` finds the entry by `UpgradeCode` (not by DisplayName, which the old NSIS product shares) and asserts the name, publisher, numeric DisplayVersion, `Comments` carrying the full project version, help/about links, icon, an `EstimatedSize` over 50 MB, and an `UninstallString` that runs msiexec — then that the entry is gone after uninstall and that an upgrade leaves exactly one of them
- [ ] **Verify file associations end to end** — double-clicking a `.sm2d` opens seamly2d and loads the pattern; `.smis` / `.smms` open seamlyme; the correct icons show in Explorer; the associations are removed on uninstall and survive an upgrade-over-install. (SeamlyLayout still has no association of its own — its input is the `.pieces.svg` handoff, which cannot be registered distinctly from plain `.svg`.) — *authoring verified*: all three extensions map to their ProgId, each ProgId has `shell\open\command` = `"[#<exe>]" "%1"` and a `DefaultIcon`. On a real install `test_msi_install.ps1` reads the three `HKLM\SOFTWARE\Classes` chains back, resolving `shell\open\command` against the actual install directory, and opens a real `.sm2d` with `Start-Process` — which goes through ShellExecute, the same route Explorer takes for a double-click — then asserts all three are gone after uninstall. One limit worth knowing: a per-user `UserChoice` overrides the machine-wide registration, so the effective association is *reported* rather than asserted; HKLM being correct is all an installer can be held to
- [X] **Offer optional desktop and taskbar shortcuts at install time** — done, with two decisions recorded in `scripts/packaging/windows/README.md`: **one** checkbox ("Create desktop shortcuts for Seamly2D and SeamlyMe", default on, property `SEAMLYDESKTOPSHORTCUTS`, overridable as `msiexec … SEAMLYDESKTOPSHORTCUTS=0`) rather than one per app, and **no shortcut for SeamlyLayout** because it is launched by seamly2d with a `.pieces.svg` argument and a bare desktop launch shows an empty canvas. **Taskbar pinning is not offered at all** — Windows 10 and later block programmatic pinning (the `taskbarpin` verb is unavailable to third parties, there is no MSI/WiX element, and the only supported route is OEM/enterprise layout-modification XML applied by Group Policy or imaging, which a user-run MSI cannot use), so a checkbox would silently do nothing. The checkbox lives on a spawned `SeamlyShortcutsDlg`; the components are conditioned on the property and validated with ICE43/ICE57 suppressed (both are false positives for a `perMachine` package — see the README)
- [X] **Prompt for elevation with UAC** — verified in the built package, not merely intended: summary-information Word Count has the LUA bit (8) **clear** (i.e. elevation required), `Package/@Scope="perMachine"` yields `ALLUSERS=1`, and the summary template is `x64;1033`. All three are now asserted on every build. The one part that still needs a real machine is what the UAC dialog *looks like* — and its publisher line only becomes meaningful after **Task 33** signs the package
- [X] **Warn when an existing installation will be replaced** — `SeamlyPreviousInstallDlg`, shown from `InstallUISequence` at sequence 1250 (after `FindRelatedProducts` and `AppSearch`, before WixUI's first dialog at 1296) under `(WIX_UPGRADE_DETECTED OR SEAMLYNSISUNINSTALLSTRING) AND NOT Installed`. One paragraph per case, shown/hidden by `ControlCondition`, plus an always-visible paragraph naming `C:\Users\<you>\seamlyData`, `AppData\Local\Seamly` and `AppData\Roaming\Seamly` and stating that neither installing nor uninstalling removes them. The NSIS install is found with `RegistrySearch Bitness="always32"` — the NSIS installer is 32-bit and never switches views, so its keys are under `WOW6432Node` and a default-view search finds nothing (confirmed against the real NSIS install on the developer PC)
- [X] Decide what to do about a detected **NSIS** install specifically — **superseded 2026-07-31 by the user's step 2a: Setup now removes the NSIS installation itself** (files, Start Menu folder, both registry keys and its ARP entry) via `RemoveNsisProgramFiles` and `RemoveNsisRegistryKeys`, justified because the MSI is a strict superset of the NSIS product. The one part of the original decision that stands is that **its interactive `uninstall.exe` is never run** — `RMDir /r $INSTDIR` would take anything else in that folder with it and Windows Installer could not roll it back. Verified on the laptop 2026-08-02: all five removal checks passed, still with no `CustomAction` in the package. The original decision, kept for the record: **detect it, name its path in the warning dialog, tell the user to uninstall it from Apps & features afterwards, and never touch it.** Running its `uninstall.exe` from a custom action was rejected: it is an interactive EXE, its uninstall section is `RMDir /r $INSTDIR` (so it would delete anything else in that folder), and Windows Installer cannot roll it back if the rest of the install fails. Leaving it is harmless — different product, different install directory — the only cost being two ARP entries named "Seamly2D", distinguishable because the NSIS one shows no version. Recorded in `scripts/packaging/windows/README.md`
- [ ] **Run the full elevated cycle** on a clean Windows x64 machine or VM: `msiexec /i` → check every item above → upgrade-over-install with a newer build → uninstall → confirm no leftover files, shortcuts, registry rows or ARP entry, and that the user data root is still intact afterwards. This also closes **Task 13**'s outstanding verify subtask. **Ready to run (2026-07-29):** the kit at `scripts/seamly-msi/task51-test-kit/` holds both MSIs, `test_msi_install.ps1`, a sample pattern and `RUN-ME-FIRST.md`; copy it to the test Windows 11 laptop and follow that page. The four phases are scripted and the residue that needs human eyes (UAC prompt, wizard page order and wording, icons) is the short list under "What still needs human eyes" in `scripts/packaging/windows/README.md`. **Run 2 (2026-08-02) completed the `Baseline`, install, `Installed`, upgrade and `Upgraded` legs; only `Removed` is outstanding**, so this and Task 13's subtask close as soon as the uninstall leg is run
- [X] Update `scripts/packaging/windows/README.md` and `README_WINDOWS_BUILD.md` with the resulting install-time UX (which prompts appear, in what order, what each one does) and with the verification checklist above, so the next person can re-run it — README.md gained an "Install-time experience (Task 51)" section (the two pages, and the seven decisions behind them, including why neither page is wired through a second `NewDialog` publish); README_WINDOWS_BUILD.md gained the wizard walkthrough, the new build step, and §3.5 recording the developer machine's Qt kit missing `qtwebchannel`/`qtpositioning`. The manual checklist now lives in README.md only, so the two files cannot drift

## Task 52 — `VSettings`' own path settings also land in an "Unknown Organization" stray file (found doing Task 34, 2026-07-26)

Task 34 fixed this defect for the **shared** common settings file (`VCommonSettings::commonSettingsOrganization()` + `mergeStrayCommonSettings()`), but the same root cause is still live in `src/libs/vmisc/vsettings.cpp`. Eight accessors build a throwaway `QSettings` from the *instance's* organization and application names:

```cpp
QSettings settings(this->format(), this->scope(), this->organizationName(), this->applicationName());
```

Since Task 15 the apps construct their settings object from an explicit settings **file path** (`VSettings(qt6Settings, QSettings::IniFormat, this)` in `Application2D::openSettings()`), and `QSettings` records neither an organization nor an application name for that constructor — both come back empty. QSettings then substitutes the literal `"Unknown Organization"` and, with an empty application name, writes an organization-level file. Confirmed on the developer machine:

```text
%APPDATA%\Unknown Organization.ini
  [paths]
  layout=G:/My Drive/seamly2d/layouts
  pattern=G:/My Drive/seamly2d
  [pattern]
  graphicalOutput=true
```

Affected keys: `paths/pattern`, `paths/layout`, `paths/seamlyLayoutApp`, `pattern/graphicalOutput` (`getPatternPath`/`SetPathPattern`, `getLayoutPath`/`SetPathLayout`, `getSeamlyLayoutAppPath`/`setSeamlyLayoutAppPath`, `GetGraphicalOutput`/`SetGraphicalOutput`). Nothing is broken for the user *today* — the same wrong file is both written and read, so the values round-trip — but they sit outside the unified `Seamly` folder Task 15 established, are shared between apps rather than per-app, and are missed by the settings migration and by the uninstall/packaging documentation. Deliberately left out of Task 34 to keep that change scoped: unlike the common file (which had to be correct for the data root and the Task 14 installer), these keys are self-consistent where they are.

- [ ] Point the eight `vsettings.cpp` accessors at the app's own settings file — they intend "this application's settings", which post-Task-15 is `this`, so plain `value()`/`setValue()` as `VCommonSettings::getLabelTemplatePath()` already does; check `VSeamlyMeSettings` for the same pattern
- [ ] Bring existing values forward from `%APPDATA%\Unknown Organization.ini` (and the platform equivalents) on first run, non-destructively — copy-if-missing, never delete the stray file — mirroring `VCommonSettings::mergeStrayCommonSettings()`
- [ ] Decide whether `paths/pattern` and `paths/layout` belong in the app file or in the shared common file alongside the other seven `paths/*` keys, and record why
- [ ] **Stop `CollectionTest` writing into the real user settings first.** `%APPDATA%\Unknown Organization.ini` on the developer machine holds `layout=…\CollectionTest\bin\tst_seamly2d_tmp` — the suite launches the real `seamly2d.exe`, which persists a layout path through these very accessors. Today the defect is what contains the damage (the write lands in the stray file); the moment the accessors point at `this`, the same test run scribbles on `%LOCALAPPDATA%\Seamly\Seamly2D\qt6_seamly2d.ini`. Give the test-launched apps their own settings location (distinct organization/application name, or `QSettings::setPath()`) **before** repointing the accessors
- [ ] Add a regression test that no Seamly settings resolve to an `"Unknown Organization"` path, so a future accessor cannot reintroduce this
- [ ] Update the settings-storage tables in `.github/README-BUILDS.md` once the location changes

## Task 54 — Rename the three `vmisc` settings files **and their classes** to SettingsCommon, SettingsSeamly2d, SettingsSeamlyMe

Rename the settings sources in `src/libs/vmisc/` so each name says which app it configures, and rename the classes with them so the pair complies with `.github/README-CODE-STYLES.md`: **class names** UpperCamelCase (the project's deliberate deviation from JSF-AV, which would demand `Settingscommon`), file names unique repo-wide, and no `v` prefix.

| Current file                       | class-match (style-guide exception) | Current class         | New class            |
| ---------------------------------- | ----------------------------------- | --------------------- | -------------------- |
| `vcommonsettings.cpp` / `.h`   | `SettingsCommon.cpp` / `.h`     | `VCommonSettings`   | `SettingsCommon`   |
| `vseamlymesettings.cpp` / `.h` | `SettingsSeamlyMe.cpp` / `.h`   | `VSeamlyMeSettings` | `SettingsSeamlyMe` |
| `vsettings.cpp` / `.h`         | `SettingsSeamly2D.cpp` / `.h`   | `VSettings`         | `SettingsSeamly2D` |

**Class-rename scope measured 2026-07-26:** `VCommonSettings` **447 occurrences in 17 files**, `VSettings` **147 in 18 files**, `VSeamlyMeSettings` **25 in 9 files** (`src/`, all extensions). Plus the translations: `tr()` contexts are keyed on the class name, so all **22 `share/translations/seamly2d_*.ts`** files carry a `<name>VCommonSettings</name>` context (8 messages) and a `<name>VSettings</name>` context (2) — **~220 already-translated strings** that go obsolete unless the contexts are renamed with the classes. `VSeamlyMeSettings` has no translation context.

**Scope measured 2026-07-26:** **101 files under `src/`** `#include` one of the three headers, in two forms — the in-directory `#include "vcommonsettings.h"` and the sibling-library `#include "../vmisc/vsettings.h"` form, which resolves only because every `.pro` adds `INCLUDEPATH += $$PWD/../../libs/vmisc`. `src/libs/vmisc/vmisc.pri` is the **only** build file naming them (SOURCES lines 5/8/9, HEADERS lines 24/27/28) — no other `.pro`/`.pri`/workflow lists these sources, so the build wiring is a six-line change.

**Do files and classes in one commit, not two.** Splitting them means a middle state where `settings_common.h` declares `VCommonSettings` — exactly the file/class mismatch the style rule exists to prevent — and it doubles the churn through the same ~600 call sites.

- [ ] **Settle the file-name form first** (A class-match `SettingsCommon.h` vs B snake_case `settings_common.h`), plus the two smaller calls above (brand casing; `VSettings` → `SettingsSeamly2D`); record the decision in `.github/README-CODE-STYLES.md` if it needs sharpening, since every future rename follows it
- [ ] Rename all six files with `git mv` (not delete + add) so history and `git blame` follow the rename
- [ ] Update the six entries in `src/libs/vmisc/vmisc.pri` (SOURCES 5/8/9, HEADERS 24/27/28)
- [ ] Update every `#include` across the 101 files — both the in-directory and the `../vmisc/…` form — then confirm with a repo-wide grep that no `vcommonsettings.h` / `vsettings.h` / `vseamlymesettings.h` include remains anywhere under `src/`
- [ ] Include the test suite in that sweep: `src/test/Seamly2DTest/tst_dataroot.{h,cpp}` is the only test that includes these headers (and uses `VCommonSettings` heavily), so a missed include there fails only the test build, not the app build
- [ ] Rename the include guards to match the new file names — `VCOMMONSETTINGS_H` → `SETTINGS_COMMON_H`, `VSETTINGS_H` → `SETTINGS_SEAMLY2D_H`, `VSEAMLYMESETTINGS_H` → `SETTINGS_SEAMLYME_H` (each at lines 53-54 of its header)
- [ ] Rename the three classes at every occurrence (~620 across 25 distinct files): the `class X : public Y` declarations, constructors/destructors, every forward declaration (`class VSettings;`), member and pointer types (`VSettings *Seamly2DSettings()`, `VCommonSettings *settings`), and every static/qualified call (`VCommonSettings::…`). `VSettings` is a whole-word match — nothing else contains it — so use word-boundary, case-**sensitive** replacement and never touch the lowercase `settings` identifiers that surround them
- [ ] Rename the `tr()` contexts in all 22 `share/translations/seamly2d_*.ts` files (`<name>VCommonSettings</name>` → `SettingsCommon`, `<name>VSettings</name>` → `SettingsSeamly2D`) in the same commit, or the ~220 existing translated strings in those contexts go obsolete. Verify afterwards by running `lupdate` and confirming it reports no newly-obsolete messages in these contexts
- [ ] Update the `@file` line in each of the six license-header blocks (e.g. `//  @file   vcommonsettings.h`), and the `@brief`/`@class` text of anything that names the old class, leaving the existing `@author`/`@date`/copyright lines as they are
- [ ] Update the docs that name these paths **or classes** — `.github/README-BUILDS.md:17` (`VSettings`, `src/libs/vmisc/vsettings.cpp`) and `:77` (`VCommonSettings::dataRoot()` and the rest of that API row) at minimum — and decide whether historical entries (`project-docs/TODO_COMPLETED.md`, `SESSION_HANDOVER.md`, `project-docs/TODO_SEAMLY2D.md` Task 42, `project-docs/TODO_SEAMLYME.md` Task 43) get rewritten or left as the record of what things were called at the time; record the decision either way
- [ ] Amend Task 52 above — it points at "the eight `vsettings.cpp` accessors" and at `VCommonSettings::mergeStrayCommonSettings()` / `getLabelTemplatePath()` — so whoever picks it up looks for `settings_seamly2d.cpp` and `SettingsCommon`
- [ ] Build and test locally: `scripts/sd.ps1` plus the test binaries (`scripts/st.ps1` runs only one of the four that CI runs via `make check` — run the others too). Wipe the shadow-build tree first; a stale `Makefile`/object tree can link an old object and mask a missed include (Task 46)
- [ ] Confirm CI stays green on all three workflows that compile these sources (`ci.yml`, `windows-msi.yml`, and `seamlylayout-ci.yml` only if it pulls the parent libs)

## Task 55 — Refresh the developer install and build instructions in `.github/README-DEVELOPER.md`

Rewrite the **"Recommended installation for development on all platforms (Linux, MacOSX, Windows)"** section (line 50) and the **"Building Seamly"** section (line 116) of `.github/README-DEVELOPER.md` so they describe how Seamly is actually built today on a local PC. (Filename note: the request says `README_DEVELOPER.md`; the file in the tree is `.github/README-DEVELOPER.md`, with hyphens.) `.github/README-BUILDS.md` is the maintained build knowledge base — the developer README should give the short, correct path for each platform and link there for depth rather than duplicating it.

**Defects found reading the current text (2026-07-26):**

- **Lines 54-55 contradict themselves and the project**: "MS Visual Code Community Edition 18 (for the IDE)" + "MS Visual Studio 2022 (for the compiler)". `CLAUDE.md` says the local toolchain is Qt 6.11.1 `msvc2022_64` + **VS 18 Community** MSVC (`vcvars64.bat`) while CI uses MSVC 2022 — settle what is required versus what is merely known-good, and say it once
- **The Qt module list names no WebEngine-family module at all** — fixed on 2026-07-26 (Qt WebChannel + Qt Positioning under "Additional Libraries", with the reason), then **lost in the `develop` merge**, which also dropped Qt WebEngine itself and the "don't select Qt Design Studio" warning. Without all three of `qtwebengine` / `qtwebchannel` / `qtpositioning`, `find_package(Qt6 … WebEngineQuick)` fails at configure time naming WebEngine rather than the module actually missing (Task 44, `CLAUDE.md` build notes). This is the single most common local-setup failure on the project
- **No Rust toolchain prerequisite** (rustup/cargo) even though seamlyLayout is half Rust and CMake 3.30.5 + Ninja 1.12.1 are already listed
- **None of the scripts that people actually run are mentioned**: `scripts/sd.ps1` (debug build/run), `scripts/st.ps1` (tests), `src/app/seamlylayout/build.ps1` and `qd.ps1` (the daughter app's own build), `scripts/packaging/windows/smsi.ps1` (MSI)
- **The Windows build block (lines 131-137)** is fenced `` ```bash `` for `cd …\build`, `qmake ..\Seamly.pro`, `nmake`; it does not say the shell must be a VS developer environment (`vcvars64.bat`), does not mention jom (the toolchain of record), and does not warn that a bare `qmake` on a machine with Qt Design Studio resolves to its reduced Qt with no `mkspecs/` (Task 47)
- **Line 108 links Qt 5 documentation** (`doc.qt.io/qt-5/windows.html`) for a Qt 6 project
- **The Xpdf/pdftops instructions are duplicated and misfiled** — line 104 (under *MacOSX*) and line 112 (under *Windows*) both give the same Windows-and-MacOS sentence
- **Lines 139-157 (Linux, MacOS)** give bare `qmake`/`make`/`sudo make install` with no statement of which Qt or compiler is supported, or how to get Qt 6.11.1 on those platforms; macOS covers only `CONFIG+=macSign`
- **Line 80** ("*Don't select Qt Design Studio -- it is based on Qt 6.8.3*") needs re-verification against the current installer, and should carry the *reason* from Task 47 (its stripped Qt on `PATH` breaks builds), not just the version

- [ ] Add the missing `qtwebchannel` / `qtpositioning` modules to the Qt component list, with the reason and the recovery path via the Maintenance Tool — **done 2026-07-26 in `09da7801e0`, then lost in the `develop` merge `1d74f7e18a`**, which took develop's rewrite of this file. Re-apply it; the current file lists no WebEngine-family module at all, so seamlyLayout's prerequisites are undocumented
- [ ] Rewrite the rest of the "Install on all platforms" list: one clear IDE/compiler statement, CMake/Ninja, and rustup/cargo for seamlyLayout
- [ ] Rewrite the three platform-specific install subsections (Linux, MacOSX, Windows) — current package/tool lists, Qt 6.11.1 acquisition per platform, and the pdftops/Xpdf step stated once under the right heading
- [ ] Rewrite "Building Seamly" to cover all three apps: the qmake/jom parent build (seamly2d + seamlyme from `src/Seamly.pro`), the seamlyLayout CMake + Cargo build, and which script drives each on Windows
- [ ] State the Windows shell requirement explicitly (VS developer environment / `vcvars64.bat`), use a PowerShell-appropriate fence instead of `` ```bash ``, and add the Design Studio `qmake`/`QMAKE` caveat from Task 47
- [ ] Refresh the Linux and macOS build blocks: supported Qt/compiler, the `qmake6` note (already present, line 149) kept, macOS signing/notarizing kept and cross-referenced to the packaging docs
- [ ] Add a short "how to run the tests" pointer (`scripts/st.ps1` and the full `make check` set CI runs) so a new contributor can verify a build
- [ ] Fix the stale link on line 108 (Qt 5 → Qt 6) and sweep the section for any other Qt 5-era links
- [ ] Cross-link `.github/README-BUILDS.md` (toolchains, packaging, settings/data locations) and `.github/workflows/README_WORKFLOWS.md` instead of restating them; keep CI's Qt version named in exactly one place
- [ ] Verify the instructions by following them literally on this Windows PC (and, where they can only be reviewed, say so) — a doc that has not been walked through is the reason for this task

## Task 60 — Rethink where user data lives: a `Seamly` brand tree, and separate documents from application state (user proposal, 2026-07-31)

Supersedes the **default locations** in Task 14 and Task 34/53. It does not supersede Task 14's *migration mechanics* (inventory, merge-never-overwrite, copy → verify → remove, fail-safe abort) — those subtasks stand and this task changes what they migrate **to**.

**The user's reasoning, kept because it is the rationale for the whole change:** `Seamly` represents the complete product family, "Data" is redundant when the parent location already says these are application files, PascalCase matches the product names and is safe on macOS and Linux, `seamlyData` mixes naming conventions, and keeping `seamly2d` wrongly implies SeamlyMe and SeamlyLayout belong to Seamly2D. **The principle behind it — do not put user-created documents and internal application state in the same directory — is the most valuable part and the current design violates it.**

### Settled

- **The brand-level parent folder is `Seamly`.** Not `seamlyData`, not `seamly2d`, not any variation.
- **Migration is copy-and-verify, and the legacy tree is left intact**, because a user may need to roll back to an earlier release. Never a bare rename. (Confirmed 2026-07-31; matches Task 14's existing cross-volume subtask.)
- **The installer does not do this.** A per-machine MSI's server side runs as LocalSystem, so a per-user path resolves to the SYSTEM profile and would only ever cover whoever ran setup — and macOS and Linux have no MSI at all. It is app code on first run, where adoption happens today.

### The specification (user, 2026-07-31)

**The boundary that drives everything:** users see and manage `Documents/Seamly`; internal configuration, caches, logs and recovery stay in the operating system's application-data locations. Two trees, two different groupings — **application data by application, user documents by type**. (These are not competing taxonomies, as an earlier reading of the proposal assumed; they apply to different trees.)

| Platform | Configuration                                                                                                | Cache                                  | User documents                      |
| -------- | ------------------------------------------------------------------------------------------------------------ | -------------------------------------- | ----------------------------------- |
| Windows  | `%APPDATA%\Seamly\<app>\`                                                                                  | `%LOCALAPPDATA%\Seamly\<app>\Cache\` | `%USERPROFILE%\Documents\Seamly\` |
| Linux    | `$XDG_CONFIG_HOME/Seamly/<app>/`                                       | `$XDG_CACHE_HOME/Seamly/<app>/` | `$XDG_DOCUMENTS_DIR/Seamly/`         |                                     |
| macOS    | `~/Library/Application Support/Seamly/<app>/`                                                              | `~/Library/Caches/Seamly/<app>/`     | `~/Documents/Seamly/`             |

Also specified: logs and recovery under `%LOCALAPPDATA%\Seamly\<app>\{Logs,Recovery}\` on Windows, `~/.local/state/Seamly/<app>/{Logs,Recovery}/` on Linux, `~/Library/Logs/Seamly/<app>/` on macOS; a `Shared\` sibling for genuinely shared internal data; and the document tree as `Documents/Seamly/{Projects, Patterns, Measurements, Layouts, Templates, Exports}`.

**Linux must resolve `XDG_DOCUMENTS_DIR` rather than assuming the folder is literally named `Documents`** — localized systems rename it. Same for the other XDG variables when a user overrides them.

**Relocatability is unaffected**: `paths/dataRoot` stays the single setting the document tree derives from, because `G:\My Drive\…` is the whole reason it exists and `Documents` is often OneDrive-redirected already. Only the *default* changes.

### What this changes about today's layout, and what is still open

Settings today are **not** in the specified shape: `%APPDATA%\Seamly\qt6_common.ini` is a shared file at the parent level, and `%LOCALAPPDATA%\Seamly\<app>\qt6_<app>.ini` holds per-app *configuration* rather than cache. Moving to the table above means config consolidates under `%APPDATA%\Seamly\<app>\` and `%LOCALAPPDATA%` becomes cache/logs/recovery only — a settings migration in its own right, on top of the document migration. Task 15 established the current shape, so this supersedes it.

### Answered by the user, 2026-07-31 — these close the questions above

- **Copy the tree WHOLESALE. Do not enumerate known subfolders.** Users have added their own directories under `seamly2d` — the user's own machine has `Projects` and `bodyscans` among them — so anything that migrates a fixed list silently strands the rest. This single rule also disposes of the "four folders have no home" problem: nothing is re-sorted, everything comes across.
- **The structure is copied as-is and only the ROOT is renamed**, to `Seamly`. Existing subfolder names are preserved, so `measurements/` keeps `individual`, `multisize` **and any other directory found under it**. PascalCase subfolder names are therefore *not* part of this task — if wanted, that is a separate change with translation consequences (`vcommonsettings.cpp` names them through `tr()`).
- **`backups` stays where it is for now.** It arguably belongs under `Recovery\`, and that is agreed in principle, but the behaviour is deliberately not being changed yet.
- **`images` and `backups` belong to the Seamly2D application** — noted; whether `images` should ship *with the install* rather than living in the user tree is a separate packaging question, and wholesale copying makes it moot for migration purposes.
- **`~/.local/state/…` is not needed**, so the missing Qt `StateLocation` is a non-issue. Logs and recovery stay where they are today.
- **`Projects` is a real user folder**, not a concept the apps need to invent — the user already has one. Nothing to define; wholesale copy carries it.
- **macOS `.plist` preferences: not adopted.** `QSettings::IniFormat` everywhere, so the three platforms behave identically.

### Implementation decision (2026-07-31)

**The migration lives in the applications, not the installer**, on the user's explicit "whatever approach you believe is efficient and also is easy to troubleshoot and maintain". A per-machine MSI custom action can only reach the *installing* user's profile (its server side runs as LocalSystem), cannot be unit-tested, does not exist on macOS or Linux so the logic would be written twice, and fails in the hardest place to diagnose. The same code in `VCommonSettings` runs for every user on every platform, is testable against `QTemporaryDir`, and can be logged and re-run.

**Verified on a real profile 2026-08-02** (Task 51 run 2, Windows 10 laptop): the legacy `~/seamly2d` tree was copied wholesale to `Documents\Seamly` on first launch — all eight existing folders including the user-added `bodyscans`, plus the nine standard ones, so `images` finally exists — the legacy tree was left intact and gained only `MIGRATED-TO-SEAMLY.txt` naming the new root and the date. The copy path, the marker and the seeding all behave as designed on a profile that no test could stand in for.

- [X] `getDefaultDataRoot()` returns `<DocumentsLocation>/Seamly` (`QStandardPaths::DocumentsLocation`, which resolves `XDG_DOCUMENTS_DIR` on Linux and the known-folder API on Windows)
- [X] New migration function: recursive copy of the **entire** legacy tree, merge-never-overwrite (skip and report collisions), verify each file by size after copy, abort with the source intact on any failure, and never delete the source — `migrateDataTree()`, which additionally refuses a destination nested inside the source
- [X] Leave the legacy tree in place and drop a marker file in it naming the new root and the date, so it is not offered again and is obviously stale to a human — `markDataTreeMigrated()` / `dataTreeWasMigrated()`
- [X] Wire it into `initializeDataRoot()`'s first-run path in place of adopt-in-place; a configured root is still honoured untouched — via `migrateAdoptedLegacyTree()`, called from `Application2D::openSettings()` and `ApplicationME::openSettings()` rather than from `initializeDataRoot()` itself, because the unit tests call that and it would resolve against the real home directory (the Task 34/53 rule). `paths/dataRoot` is repointed only *after* the copy verifies
- [ ] **A multi-gigabyte copy cannot block startup silently** — the user's own tree is ~17 GB on a cloud drive. Decide the UX (progress, cancel, or defer-and-offer) before this ships. **Not exercised by the 2026-08-02 run**, whose legacy tree was four files: the copy was instant, so the one risk this subtask exists for remains entirely untested
- [X] Unit-test against `QTemporaryDir` only — never a path under `QDir::homePath()` — `TST_DataRoot` is 28 cases, six of them new for the migration (whole-tree copy including unknown folders, never overwriting, source left intact, destination-inside-source refused, marker written, resolve-then-seed)
- [ ] Write the chosen layout into `.github/README-BUILDS.md` and update the installer's user-data dialog text, which names `seamlyData` today — README-BUILDS.md done; **the dialog is not**, and the 2026-08-02 run shows the stale `C:\Users\<you>\seamlyData` on screen during the upgrade

### Subtasks

- [X] Settle the two recommendations above with the user, then write the chosen layout into `.github/README-BUILDS.md` before any code changes
- [X] New per-platform defaults for the **document** root: Windows `%USERPROFILE%\Documents\Seamly`, macOS `~/Documents/Seamly`, Linux `~/Documents/Seamly` (falling back to `$XDG_DOCUMENTS_DIR`); replaces `getDefaultDataRoot()`'s `~/seamlyData` — one `QStandardPaths::DocumentsLocation` call covers all three
- [X] Decide whether the nine subfolder names become PascalCase (`Patterns`, `Measurements/Individual`, ...) — **decided: no.** Only the root is renamed; existing subfolder names are preserved as-is, so this is out of scope and its `tr()` consequences are untouched
- [ ] Detection must recognise **three** legacy roots now — `~/seamly2d`, `~/seamlyData`, and an already-migrated `Documents\Seamly` — and pick correctly when more than one exists. The 2026-08-02 run exercised only the `~/seamly2d`-alone case; the machine had no `seamlyData`
- [X] Mark a migrated legacy tree so it is not offered again and is obviously stale to a human — a marker file naming the new location and the date, rather than deleting anything
- [X] Reuse Task 14's migration mechanics rather than writing a second copier; if Task 14 is not yet built, build it here and let Task 14 consume it — built here as `migrateDataTree()`; Task 14 consumes it
- [ ] `pruneEmptyLegacyDataRoot()` must not remove a tree that is now merely *stale* rather than empty — re-check its conditions against the new three-root world
- [X] Unit-test against `QTemporaryDir` only, never a path under `QDir::homePath()` — it cannot be redirected on Windows
- [ ] Update `.github/README-BUILDS.md`, `scripts/packaging/windows/INSTALL_DECISION_FLOW.md` and the installer's user-data dialog text, which names `seamlyData` today — README-BUILDS.md and INSTALL_DECISION_FLOW.md are done; **the dialog text is not** and is visible to users, as the 2026-08-02 run confirmed (**Task 64**)

## Task 61 — `test_msi_install.ps1`: wrong install-state constants, a snapshot taken too early, and a sample pattern that cannot load (found doing Task 51 run 2, 2026-08-02)

Three defects in the checker, not in the package. The first has now produced false failures in **both** laptop runs, which is worth stating plainly: a checker that cries wolf costs more than no checker at all, because it trains the reader to skim the failure list.

- [ ] **`Get-AdvertisedShortcutTarget` uses the wrong `INSTALLSTATE` values.** [test_msi_install.ps1:455-457](scripts/packaging/windows/test_msi_install.ps1#L455-L457) treats states **4/5** as the ones yielding a usable path; the constants are `INSTALLSTATE_LOCAL = 3` and `INSTALLSTATE_SOURCE = 4` (`INSTALLSTATE_DEFAULT = 5` is an input value, never returned by `MsiGetComponentPath`). All three Start Menu shortcuts returned 3 — installed locally — and were reported as failures in both phases of run 2. Fix to `-eq 3 -or -eq 4`, and name the constants rather than writing bare integers, which is how the error survived review
- [ ] **Regression-guard it:** the fix is one comparison, so add a check that the *pass* path is reachable — assert that a resolved advertised shortcut yields a non-empty `ComponentPath` under the install directory, so a state constant that never matches fails loudly instead of silently reporting every shortcut broken
- [ ] **The user-data inventory is snapshotted before the migration can have happened.** The `Installed` phase runs immediately after `msiexec /i`, before any app has started, so `Documents\Seamly` does not yet exist; at `Upgraded` the comparison therefore reports "did not exist at Installed — nothing to preserve" for the one tree the upgrade most needs to protect. Re-snapshot after first run, or add an explicit `-Phase Migrated` between the app launch and the upgrade
- [ ] **`sample-pattern.sm2d` depends on a measurement file the kit does not carry** (`./2025-06-08-Sue.smis`), so opening it through the association leaves seamly2d on a "locate the measurement file" prompt. The check passes — it only asserts the process starts — but it cannot prove the pattern *loads*, which is what the subtask it serves actually claims. Ship a self-contained pattern with no external measurement dependency, and assert the loaded document rather than the running process
- [ ] Re-run the affected phases and confirm the failure list is empty before Task 51's uninstall leg

## Task 62 — ARP `DisplayIcon` is never written to the registry (diagnosed doing Task 51 run 2, 2026-08-02)

The authoring is correct and has been all along: `ARPPRODUCTICON = seamly2d.ico` is in the Property table, the Icon table has the matching row, and the verbose log shows `ProductInfo(… ProductIcon=seamly2d.ico …)` executing. Windows Installer records that as **product metadata**, reachable through `MsiGetProductInfo`, and does **not** write a `DisplayIcon` value into the `Uninstall` key.

The consequence is visible and was reported independently by the tester: the legacy `appwiz.cpl` applet ("Uninstall or change a program") shows the icon and the publisher, while the Windows Settings "Apps & features" page shows neither, because Settings reads the registry values directly.

- [ ] Author `DisplayIcon` explicitly as a registry value under the product's `Uninstall` key, pointing at the installed `seamly2d.exe` (with an index if the icon is not the first resource). Keep `ARPPRODUCTICON` — it is what the legacy applet uses
- [ ] Check whether `Publisher` needs the same treatment; the tester saw it missing in Settings and present in `appwiz.cpl`, which is the same symptom and may share the cause
- [ ] Assert the registry value in `test_msi_authoring.ps1` and its runtime value in `test_msi_install.ps1`; the current check reads the registry and is correct to fail today
- [ ] Verify on the test machine in **both** applets, since they demonstrably disagree

## Task 63 — Brand the installer and the products for the family, not for Seamly2D (tester, 2026-08-02)

Every wizard page still says "Seamly2D" while the package installs three applications. Cosmetic individually, but it is the user's first contact with the product and it currently misrepresents what is being installed.

- [ ] Wizard strings → **"Seamly"**: the window title (`Seamly2D Setup`), "Welcome to the Seamly2D Setup Wizard", "Ready to install Seamly2D", "Installing Seamly2D", "Completed the Seamly2D Setup Wizard", and the uninstall's "Please wait while Windows configures Seamly2D"
- [ ] The welcome body must name all three: "The Setup Wizard will install Seamly2D, SeamlyLayout, and SeamlyMe on your computer. Click Next to continue or Cancel to exit the Setup Wizard."
- [ ] EULA: "Seamly2D application family" → "Seamly application family"
- [ ] **Publisher and copyright: "Seamly2D Project" → "Seamly Project" — scope is package-and-About ONLY** (user, 2026-08-02). That means `Manufacturer`/`ARPPUBLISHER` in `seamly-family.wxs`, the three executables' version resources, and the About boxes. **Source-file copyright headers are explicitly out of scope and stay "2026 Seamly2D Project"**, as `CLAUDE.md` specifies — a repo-wide header rewrite would touch every file and must not ride along with an installer fix. Do not "helpfully" extend this while editing nearby code
- [ ] Update the assertions in `test_msi_authoring.ps1`, which pin several of these strings
- [ ] Confirm the wizard by eye — string changes are exactly the class of change that authoring tests pass and users still see wrong

## Task 64 — Rewrite the previous-install dialog: it is too long and it names a data folder that no longer exists (tester, 2026-08-02)

`SeamlyPreviousInstallDlg` works — it appeared correctly in both runs, in the right position — but its text is stale and over-long. The stale part is not cosmetic: **the always-visible paragraph tells the user their work lives in `C:\Users\<you>\seamlyData`, which Task 60 replaced with `Documents\Seamly`.** Run 2 caught it on screen during the upgrade. This closes Task 60's last documentation subtask.

- [ ] Replace `C:\Users\<you>\seamlyData` with `C:\Users\<you>\Documents\Seamly` throughout, and re-check the `AppData\Local\Seamly` / `AppData\Roaming\Seamly` sentence against what Task 60 actually ships
- [ ] Shorten the NSIS paragraph to one sentence: "An older Seamly2D version was found in `C:\Program Files (x86)\Seamly2D`."
- [ ] **Drop the advice to move your own files out of that folder.** Users have no reason to have put anything in Program Files, and the sentence invites them to worry about a problem they do not have. (It was written when Setup left the NSIS install alone; step 2a now removes it, so the advice is also no longer true to the behaviour)
- [ ] Make the "Your own work is not touched …" paragraph much terser
- [ ] Fix the geometry while the file is open — `BannerLine`/`BottomLine` are `Width="373"` on a `Width="370"` dialog, raising error 2826 twice per install (Task 51's second subtask; three characters, same file)
- [ ] Update the wording assertions in `test_msi_authoring.ps1` and the transcript of the dialog in `scripts/packaging/windows/INSTALL_DECISION_FLOW.md`

## Task 65 — Destination-folder page: wording, and whether the install folder is `SeamlyApps` or `Seamly` (tester, 2026-08-02)

The tester asks for "Install Seamly2D to" → **"Install Seamly applications to the 'Seamly' subdirectory under"**, with the edit box showing `C:\Program Files\`.

**This reverses a settled decision and needs confirmation before any code changes.** `INSTALLFOLDER` was deliberately named `SeamlyApps` under `ProgramFiles64Folder` at the start of the install-layout rework, and run 2 verified `C:\Program Files\SeamlyApps\` end to end. Changing it to `Seamly` invalidates both staged MSIs, every path assertion in both test scripts, the flow chart, and the READMEs — and leaves anyone who installed a test build with an orphaned directory.

There is also a wrinkle in the requested wording: showing `C:\Program Files\` in the edit box while silently appending `Seamly` means the control no longer displays the path it edits, so Browse and hand-typed paths behave surprisingly. If the folder is renamed, the honest presentation is an edit box showing the full `C:\Program Files\Seamly`.

- [ ] **Confirm with the user: `SeamlyApps` or `Seamly`?** Everything else here depends on the answer
- [ ] If renamed: change `INSTALLFOLDER`, both test scripts, `INSTALL_DECISION_FLOW.md`, `scripts/packaging/windows/README.md`, `README_WINDOWS_BUILD.md`, and rebuild the test kit
- [ ] Retitle the page label to name the family rather than Seamly2D, and decide whether the edit box shows the full target path (recommended) or the parent
- [ ] Consider whether an existing `SeamlyApps` install should be detected and removed like the NSIS one, or left orphaned — only relevant to machines that ran a test build

## Task 66 — Apps & features lists only "Seamly2D" for a package that installs three applications (tester, 2026-08-02)

Neither applet shows SeamlyMe or SeamlyLayout. This is inherent to the design — one MSI product, one `ProductCode`, one ARP entry — not a defect in the authoring.

A single product genuinely cannot offer three independently uninstallable entries.

**Decided by the user, 2026-08-02: keep one entry and rename it "Seamly".** The alternatives were display-only ARP entries for SeamlyMe and SeamlyLayout — visible but misleading, since uninstalling "SeamlyMe" would remove all three — and splitting into three MSI products or a Burn bundle, which is a far larger change. One entry named for the family is the honest description of what is installed, and it pairs with **Task 63**'s branding pass.

- [X] **Decide** — one entry, renamed **"Seamly"**
- [ ] Change `ProductName`/`ARPDISPLAYNAME` in `seamly-family.wxs` from `Seamly2D` to **`Seamly`**, and make `ARPCOMMENTS` name all three applications so the entry says what it installs
- [ ] **`test_msi_install.ps1` finds the product by `UpgradeCode`, not by DisplayName** — that was a deliberate choice, because the old NSIS product also called itself "Seamly2D", so the rename does not break the lookup. But every assertion on the *value* `Seamly2D` must change, as must `test_msi_authoring.ps1`'s DisplayName assertions
- [ ] Check the NSIS-detection path still works: it keys off the NSIS registry keys, not the display name, so it should be unaffected — confirm rather than assume
- [ ] Verify in **both** applets on the test machine, together with Task 62's icon and publisher fix, since all three are the same ARP entry
- [ ] Renaming the product does **not** change `UpgradeCode`, so an installed test build still upgrades cleanly — verify that on the next laptop run rather than trusting it

## Task 67 — First-run modal dialogs block the main window and swallow a pattern passed on the command line (found doing Task 51 run 2, 2026-08-02)

On a fresh install the tester saw a welcome dialog for Seamly2D and one for SeamlyMe, and reported that when a `.sm2d` was opened through its file association **"the seamly2d preferences dialog opened but the application window did not open"** and the pattern did not load.

Part of this is the checker: it starts each app and stops it a couple of seconds later, so windows flash past. But the file-association case is a real user path — double-clicking a pattern on a newly installed machine — and a modal first-run dialog standing in front of the main window means the document silently does not open. SeamlyLayout, by contrast, started with no preferences dialog at all, so the three apps do not agree on first-run behaviour.

- [ ] Reproduce outside the checker: fresh profile, install, then double-click a `.sm2d` **as the very first launch**. Confirm whether the pattern loads once the dialog is dismissed or is dropped entirely
- [ ] A file passed on the command line must survive first-run dialogs — queue it and open it after the dialog closes, or suppress the dialog when the app was launched with a document
- [ ] Settle what each app shows on first run. The tester's stated expectation is that each dialog **waits for OK or Cancel**; today Seamly2D and SeamlyMe show one and SeamlyLayout shows none
- [ ] Check the same path for `.smis`/`.smms` into SeamlyMe
- [ ] Once settled, teach `test_msi_install.ps1` to dismiss or suppress the first-run dialog so the association check can assert the document *loaded* rather than that the process started (shares the Task 61 subtask on the sample pattern)
