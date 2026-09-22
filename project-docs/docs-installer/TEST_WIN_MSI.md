# TEST_WIN_MSI.md

Scripted install/upgrade/uninstall test for the Windows x64 Seamly MSI.
Runs `packaging\windows\local_install_msi.ps1` around two `msiexec` calls.
Covers `packaging/windows/smsi.wxs`. See
`packaging/windows/README_WINDOWS_INSTALLER.md` for the full installer design.

## A. Prerequisites

- [ ] 1. Two MSI builds, different `-Version` values (2 CI runs or 2 local builds):
  - `seamly-x64-older.msi`
  - `seamly-x64-newer.msi`
- [ ] 2. Copy `local_install_msi.ps1` beside the two MSIs. It is standalone.
- [ ] 3. A sample pattern file, `sample.sm2d`, in the same directory.
- [ ] 4. An elevated PowerShell 5.1 shell. Unelevated misses HKLM rows.
- [ ] 5. No prior Seamly install on the test machine. If one exists, remove it with
      `local_reset_environment.ps1` first.

## B. Test Sequence

Run in order. Each `local_install_msi.ps1` call reads and writes the shared
state file (`%LOCALAPPDATA%\seamly-msi-install-test\state.json` by default),
so later phases assert against earlier ones.

- [ ] 1. `.\local_install_msi.ps1 -Phase Baseline`
  - Asserts the Seamly suite is not already installed.
  - Records the starting user-data inventory and the legacy-NSIS state.
- [ ] 2. `msiexec /i seamly-x64-older.msi`
  - Interactive install. Walk the wizard with default settings.
- [ ] 3. `.\local_install_msi.ps1 -Phase Installed -ExpectSeamlyLayout -PatternFile .\sample.sm2d`
  - Asserts files, registry rows, seeded `.ini` settings, shortcuts, file
    associations, ARP entry, and that all three apps start.
  - Opens `sample.sm2d` through its file association and confirms seamly2d starts.
  - If a legacy NSIS install was present at Baseline, asserts it was removed.
  - Asserts no user data shrank since Baseline.
- [ ] 4. `msiexec /i seamly-x64-newer.msi`
  - Interactive upgrade install over the older build.
- [ ] 5. `.\local_install_msi.ps1 -Phase Upgraded -ExpectSeamlyLayout -PatternFile .\sample.sm2d`
  - Repeats every check from step 3.
  - Asserts exactly one MSI entry in Apps & features (no duplicate from the upgrade).
  - Asserts the installed version changed and the install directory did not.
  - Asserts no user data shrank since the Installed phase.
- [ ] 6. `msiexec /x seamly-x64-newer.msi`
  - Interactive uninstall.
- [ ] 7. `.\local_install_msi.ps1 -Phase Removed`
  - Asserts the product, registry rows, shortcuts, and file associations are gone.
  - Asserts the install directory holds no leftover files.
  - Asserts user data survived intact from both the Installed and Baseline phases.
  - Asserts a previously-removed legacy NSIS install stays removed.

## C. Pass Criteria

- [ ] Every phase prints `MSI install check passed at phase '<Phase>'.` and exits 0.
- [ ] No `FAILED` line in any phase's output.
- [ ] Review every `note` line for anything unexpected for this machine
      (legacy install present/absent, per-user file-association `UserChoice`).

A failing phase stops the sequence. Fix the defect, reset the machine, and
restart from `-Phase Baseline`. Do not skip ahead on a failure.

## D. Known Limits

The script cannot see, and these stay a manual check against
`TEST_MSI_WIN_X64_Test_Case_template.md`:

- UAC prompt appearance.
- Wizard page order and wording.
- Explorer icon rendering.

## E. Parameters Reference

`-Phase <Baseline|Installed|Upgraded|Removed>` (required), `-ExpectSeamlyLayout`,
`-NoDesktopShortcuts`, `-PatternFile <path>`, `-SkipLaunch`, `-StateFile <path>`.

Omit `-ExpectSeamlyLayout` only when testing the arm64 package (parent apps only).
