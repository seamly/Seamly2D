# Task 51 + 60 — MSI install cycle, run 2 (2026-07-31)

Copy this whole folder to the test laptop and work down this page. About 25 minutes, most of it waiting. Everything here is self-contained — no repository, build tree or Qt needed on the test machine.

## What is different from the first run

These packages are built from current source and differ from run 1 in four ways, all of which this run is meant to prove:

1. **Programs install to `C:\Program Files\SeamlyApps`**, not `\Seamly2D`.
2. **The old NSIS installation is removed during install** — its files, its Start Menu folder, both registry keys and its Apps & features entry. Its own `uninstall.exe` is never run.
3. **Your user data is COPIED to `Documents\Seamly` on first launch.** The whole tree comes across, including folders you added yourself. **Nothing is moved or deleted:** `C:\Users\susan\seamly2d` stays exactly as it is and gains a `MIGRATED-TO-SEAMLY.txt` saying where the copy went, so you can roll back.
4. **The nine standard subfolders are created** at the new root — including the `images` folder your NSIS install never made.

| File                     | What it is                                                 |
| ------------------------ | ---------------------------------------------------------- |
| `Seamly-x64-older.msi` | install this one FIRST                                     |
| `Seamly-x64-newer.msi` | the upgrade, built later so it genuinely major-upgrades    |
| `test_msi_install.ps1` | the automated checker, run between the msiexec steps       |
| `sample-pattern.sm2d`  | used to prove the`.sm2d` association opens *and loads* |

## Expected starting state

- The **old NSIS Seamly2D is installed** at `C:\Program Files (x86)\Seamly2D`
- **No** Seamly family MSI is installed
- `C:\Users\susan\seamly2d\patterns\` holds a real pattern of yours

That last one is what makes the data checks mean anything. **Do not move or delete it.**

## Before you start

- [X] **PowerShell as Administrator**, `cd` into this folder, and run everything from that one prompt.

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
Start-Transcript -Path "$env:USERPROFILE\Desktop\task51-run2.txt"
Remove-Item "$env:LOCALAPPDATA\seamly-msi-install-test" -Recurse -Force -ErrorAction SilentlyContinue
```

The last line clears any state from run 1; a stale baseline would compare against the wrong machine.

## 1. Baseline

```powershell
.\test_msi_install.ps1 -Phase Baseline
```

- [X] Must report **the NSIS installation IS present** at `C:\Program Files (x86)\Seamly2D`. If it does not, the removal path will not be exercised and there is no point continuing.
- [X] Should list `C:\Users\susan\seamly2d` with a file count.
- [X] Ends `MSI install check passed at phase 'Baseline'.`

Response:
"Seamly2D MSI install check - phase: Baseline
state file: C:\Users\susan\AppData\Local\seamly-msi-install-test\state.json

  ok      the Seamly family MSI is not already installed
  ok      HKLM\SOFTWARE\Seamly\Seamly2D does not exist yet
  note    the old NSIS installation IS present at 'C:\Program Files (x86)\Seamly2D' - the warning dialog's NSIS paragraph should appear during install
  note    user data 'C:\Users\susan\seamlyData': does not exist
  note    user data 'C:\Users\susan\Documents\Seamly': does not exist
  note    user data 'C:\Users\susan\seamly2d': 4 files, 0.1 MB
  note    user data 'C:\Users\susan\AppData\Local\Seamly': 3 files, 0 MB
  note    user data 'C:\Users\susan\AppData\Roaming\Seamly': 0 files, 0 MB

MSI install check passed at phase 'Baseline'."

Record what the old tree looks like, so the migration can be checked against it:

```powershell
Get-ChildItem "$env:USERPROFILE\seamly2d" -Directory | Select-Object -ExpandProperty Name
(Get-ChildItem "$env:USERPROFILE\seamly2d" -Recurse -File).Count
```

- [X] Write down both. `images` is expected to be **missing** right now.

Response #1:
"ndProperty Name
backups
label templates
layouts
measurements
patterns
templates"

Response #2:
"4"

## 2. Install the older package — watch the wizard

```powershell
msiexec /i Seamly-x64-older.msi
```

- [ ] Exactly **one** UAC prompt. It will say "unknown publisher" — the package is not signed yet (Task 33), which is not a failure here.
- [X] An **"An existing installation was found"** page appears *before* the welcome page, naming `C:\Program Files (x86)\Seamly2D`, saying Setup **will remove** it, and telling you to move anything of your own out of that folder first.
- [X] Welcome → License → **Destination Folder showing `C:\Program Files\SeamlyApps`** → Ready → Install.
- [X] **A "Shortcuts" page is expected NOT to appear — known defect, nothing to report.** Desktop shortcuts are created anyway, because the setting defaults to on.

TODO: Window title is 'Seamly2D Setup' --> change this to 'Seamly Setup'
TODO: Users shouldn't have added any of their own files to `C:\Program Files (x86)\Seamly2D` so don't tell users to move anything of their own out of it before continuing. The text should be shortened to 'An older Seamly2D version was found in `C:\Program Files (x86)\Seamly2D`.'
TODO: Make the text "Your own work..." much more terse.
TODO: Change 'Welcome to the Seamly2D Setup Wizard' to 'Welcome to the Seamly Setup Wizard'
TODO: Change 'The Setup Wizard with install Seamly2D on your computer. Click Next to continue or Cancel to exit the Setup Wizard.' to 'The Setup Wizard will install Seamly2D, SeamlyLayout, and SeamlyMe on your computer. Click Next to continue or Cancel to exit the Setup Wizard.'
TODO: In the EULA, Change 'Seamly2D application family' to 'Seamly application family'
TODO: In the Destination folder page, change 'Install Seamly2D to' to 'Install Seamly applications to the 'Seamly' subdirectory under'
TODO: In the Destination folder page, change 'C:\Program Files\SeamlyApps' to C:\Program Files\'
TODO: In the 'Ready to install Seamly2D' page, change 'Ready to install Seamly2D' to 'Ready to install Seamly'
TODO: In the 'Installing Seamly2D' page, change 'Installing Seamly2D' to 'Installing Seamly'
TODO: In the 'Completed the Seamly2D Setup Wizard' page, change 'Completed the Seamly2D Setup Wizard' to 'Completed the Seamly Setup Wizard'

## 3. Check the install

```powershell
.\test_msi_install.ps1 -Phase Installed -ExpectSeamlyLayout -PatternFile .\sample-pattern.sm2d
```

Checks the installed files and Qt runtime, shortcuts, registry rows, the Apps &
features entry, all three associations, that **each app actually starts**
(windows will flash up), that the sample pattern opens through its association —
and that **the NSIS install is gone**: directory, Start Menu folder, registry
key and ARP entry.

Response: Multiple windows appeared, one popup window each for the Seamly2D welcome dialog that disappeared before I could type into it, the SeamlyMe welcome dialog that disappeared before I could type into it, and the SeamlyLayout application appeared without displaying the SeamlyLayout preferences dialog.

Issue: Seamly2D did not open the sample pattern in the default pattern directory 'pattern.sm2d' as the Seamly2D application did not start--> the seamly2d preferences dialog opened but the application window did not open.

TODO: Popup the Seamly2D welcome dialog and wait for OK or Cancel to close the dialog; then popup the SeamlyMe welcome dialog and wait for OK or Cancel to close the dialog; then popup the SeamlyLayout preferences dialog and wait for OK or Cancel to close the dialog.

Then by eye:

- [X] **Apps & features lists exactly ONE Seamly2D**, with a version and
  publisher "Seamly2D Project". The old publisher-less entry is gone.
- [X] `C:\Program Files (x86)\Seamly2D` no longer exists.
- [ ] Explorer shows the Seamly icons on `sample-pattern.sm2d`.

Issue: 'Apps & features' listed 'Seamly2D' with no publisher. 'Uninstall or change a program' listed 'Seamly2D' with "Seamly2D Project" as the publisher.
Issue: 'Apps & features' did not list SeamlyMe or SeamlyLayout. 'Uninstall or change a program' did not list 'SeamlyMe' or 'SeamlyLayout'.
Issue: Change the publisher from 'Seamly2D Project' to 'Seamly Project'
Issue: Change the copyright statements from 'Seamly2D Project' to 'Seamly Project'
Issue: The 'C:\Users\Susan\seamly2d' directory exists but 'C:\users\susan\seamly' or 'C:\users\susan\seamlyData' or equivalent does not exist.
Issue: The 'C:\Users\Susan\Documents\Seamly\patterns' directory contains 'pattern.sm2d', not 'sample-pattern.sm2d'

## 4. The data migration — the new part

Start **seamly2d** from the Start Menu, let it finish loading, then close it.

Issue: The sample-pattern.sm2d pattern did not open in Seamly2d.

```powershell
"--- new root ---"
Get-ChildItem "$env:USERPROFILE\Documents\Seamly" -Directory | Select-Object -ExpandProperty Name
(Get-ChildItem "$env:USERPROFILE\Documents\Seamly" -Recurse -File).Count
Get-ChildItem "$env:USERPROFILE\Documents\Seamly\patterns"

"--- old root must be untouched ---"
(Get-ChildItem "$env:USERPROFILE\seamly2d" -Recurse -File).Count
Get-Content "$env:USERPROFILE\seamly2d\MIGRATED-TO-SEAMLY.txt"
```

Response #1:
"backups
bodyscans
images
label templates
layouts
measurements
patterns
templates"

Response #2:
"4"

Response #3:
"    Directory: C:\Users\susan\Documents\Seamly\patterns

Mode                 LastWriteTime         Length Name

---

-a----         7/30/2026   7:02 PM           1120 pattern.sm2d
"

- [ ] `Documents\Seamly` exists and contains **everything the old tree had**,
  including any folders you created yourself, plus the nine standard ones —
  `images` among them.
- [ ] **Your pattern is in `Documents\Seamly\patterns\`.**
- [ ] The old tree's file count is **unchanged** from what you recorded in step 1.
- [ ] `MIGRATED-TO-SEAMLY.txt` exists in the old tree and names the new location.

If the migration did not happen, say so — it means the copy failed and the app
fell back to using the old tree, which is the designed safe behaviour, but I
want to know about it."

Response #4:
"5"

Response #6:
"This folder has been migrated and is no longer used by the Seamly applications.
Your files were copied to: C:\Users\susan\Documents\Seamly
Date: 2026-08-02T18:36:00
Nothing here was deleted. Once you are satisfied that everything is present at the new location, this folder can be removed.
"

## 5. Upgrade over the top

```powershell
msiexec /i Seamly-x64-newer.msi
```

- [X] The existing-installation page appears again, this time with the
  **upgrade** paragraph, not the NSIS one.

Response:
"An earlier version of Seamly2D installed by this installer was found. Setup will remove its program files and install this version in their place.
Your own work is not touched. Patterns, measurements, templates, body scans, label templates, images and backups stay in your data folder - C:\Users\your name\seamlyData unless you have moved it - and your settings stay in AppData\Local\Seamly and AppData\Roaming\Seamly. Neither installing nor uninstalling Seamly2D removes any of them."
Clicked OK, then 'Welcome to the Seamly2D Setup Wizard' page appears.

Issue: Should prompt with 'C:\Users\<your name>\Documents\Seamly' instead of 'C:\Users\<your name>\seamlyData'
Issue: Change 'Install Seamly2D to' should say 'Install Seamly to'
Issue: Change prompt default 'C:\Program Files\SeamlyApps\' to 'C:\Program Files\'

```powershell
.\test_msi_install.ps1 -Phase Upgraded -ExpectSeamlyLayout -PatternFile .\sample-pattern.sm2d
```

Re-runs every check from step 3 and additionally proves exactly one entry in
Apps & features, a changed version, an unmoved install directory, and no data
loss.

Displays Seamly2D welcome dialog (disappears without input from user), SeamlyMe welcome dialog (disappears without input from user), SeamlyLayout application window (disappears without input from user), Seamly2D welcome dialog again which then opens Seamly2D which finds the C:\Users\susan\Downloads\task51-test-kit\sample-pattern.sm2d file (not in the patterns directory), and prompts for the location of measurment file './2025-06-08-Sue.smis' that is required to open the sample-pattern.sm2d file.

Response:
"...MSI install check FAILED at phase 'Upgraded' - 4 problem(s):

- ARP icon is set
- Start Menu shortcut 'Seamly2D' resolves to an installed file
- Start Menu shortcut 'SeamlyMe' resolves to an installed file
- Start Menu shortcut 'SeamlyLayout' resolves to an installed file
  "

## 6. Uninstall

```powershell
msiexec /x Seamly-x64-newer.msi
.\test_msi_install.ps1 -Phase Removed
```

- [ ] Program files, both sets of shortcuts, registry rows, ARP entry and all three associations gone.
- [ ] Apps & features lists no Seamly2D at all.
- [ ] **Both** `C:\Users\<user>\SeamlyData` and `seamly2d` still hold your files.

Issue: "Please wait while Windows configures Seamly2D" --> change 'Seamly2D' to 'SeamlyData'

```powershell
Stop-Transcript
```

- [X] Send back `task51-run2.txt` from your Desktop, plus your answers to the by-eye boxes.

## If something fails

The checker prints one line per expectation and lists failures at the end, so the failing line is the report. Send the transcript as-is.

To reset: uninstall from Apps & features, delete
`%LOCALAPPDATA%\seamly-msi-install-test`, and go back to step 1. The NSIS install cannot be restored by starting over — re-run the old installer if you need that path again. To re-test the **migration**, also delete `<parentdatadrive>:\<parentdatadirectory>` and the `MIGRATED-TO-SEAMLY.txt` marker, or the app will
correctly decline to migrate a second time.
