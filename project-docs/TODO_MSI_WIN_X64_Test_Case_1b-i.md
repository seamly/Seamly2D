# TODO — defects found by the Windows x64 MSI test case

Defects found while walking `project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`
that are not already filed in another `TODO_*.md`.

Tasks in this file begin with `MSI1b.`

A defect that already has a task elsewhere is not re-filed here. It is listed
under "Already filed, seen again" with the date it recurred.

## [ ] Task MSI1b.1 — installer dialogs log Error 2826: controls overflow the dialog by 7 pixels

Found on the 2026-09-02 pass, step 1b (fresh install of build 26.9.2.635).
Reproduced 2026-09-02 on build 26.9.2.996, this time from a **wizard**
install rather than `/quiet`: exactly 15 `Error 2826` lines, every one 7
pixels to the right. A silent install builds no dialogs, so that earlier
pass could not have seen them. The count and the offset are stable.

Every `BannerLine` and `BottomLine` control in the installer UI is 7 pixels
wider than the dialog that holds it. Windows Installer logs one Error 2826 per
control as each dialog is created:

```
Control BottomLine on dialog ExitDialog extends beyond the boundaries of the
dialog to the right by 7 pixels
```

Fifteen controls across ten dialogs are affected — the custom
`SeamlyDataDirDlg` and `SeamlyShortcutsDlg`, and the stock WixUI `WelcomeDlg`,
`LicenseAgreementDlg`, `InstallDirDlg`, `VerifyReadyDlg`, `PrepareDlg`,
`ProgressDlg` and `ExitDialog`.

**Severity: cosmetic, non-blocking.** The install completed and every verified
result was correct. The message is written at debug level; a user sees it only
with `/l*v` logging. It is filed because it is noise in every install log, and
noise hides the next real failure.

**Not caught by validation.** `wix msi validate` and
`smsi_check_authoring.ps1` both passed on this build. The overflow appears only
at run time.

- [ ] MSI1b.1.1 Measure the real cause before changing numbers. In
  `packaging/windows/smsi_ui.wxs` every custom dialog is `Width="370"` while its
  `BannerLine`/`BottomLine` is `Width="373"` (lines 170, 224, 269, 349, 416) —
  that is a 3-pixel overflow, not 7, and it does not explain the stock WixUI
  dialogs overflowing by the same 7. Find the one cause that fits all ten
  dialogs; do not just subtract 7 from each width.
- [ ] MSI1b.1.2 Apply the fix and confirm a `/l*v` install log carries no
  Error 2826 line.
- [ ] MSI1b.1.3 Add the check to `smsi_check_authoring.ps1` so a control wider
  than its dialog fails the build instead of reaching an install log.

## Already filed, seen again

| Task | Where | Last seen | Note |
| --- | --- | --- | --- |

Nothing outstanding. `Layout.7`, `Layout.10` and `SeamlyMe.3` were implemented
and verified on build 26.9.2.996 (2026-09-02); all three moved to
`TODO_COMPLETED.md`.
