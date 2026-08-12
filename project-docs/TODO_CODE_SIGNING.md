# TODO — Code Signing

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Tasks in this file are numbered with prefix `CodeSign.`

## Task CodeSign.1 — Sign the bundled executables (and DLLs) as well as the `.msi`, so the installed apps are signed too

The project already holds an **EV** code-signing certificate for **Seamly Systems, Inc.** (`.github/workflows/signing/codesign-chain.pem`) This was removed from Google Cloud. The EV CodeSigning certificate may need to be reinstalled to AWS instead of Google Cloud.

- [ ] CodeSign.1.1 Sign the three staged app exes (`scripts/seamly-msi/<arch>/exes/{seamly2d.exe,seamlyme.exe,SeamlyLayout.exe}`) after staging and before `wix build`, using the same `jsign` / Google Cloud KMS keystore, cert chain (`.github/workflows/signing/codesign-chain.pem`), and `http://timestamp.sectigo.com` timestamp as the existing MSI signing step
- [ ] CodeSign.1.2 Decide the DLL scope and sign accordingly — the Qt runtime DLLs/plugins and xerces-c staged from windeployqt output are unsigned (the MSVC CRT DLLs are already Microsoft-signed, so skip those); sign at least the app exes, and ideally the bundled Qt/xerces DLLs too, so Defender/SmartScreen sees a fully signed install tree (`parent\`, `layout\`, `exes\`)
- [ ] CodeSign.1.3 Provide the hook point without breaking local builds: either add optional signing parameters to `smsi.ps1` (a sign callback / KMS params it applies to the staged files before `wix build` and to the finished `.msi`), or split staging and `wix build` into separate phases so `ci.yml`'s `windows-msi` job can sign between them — either way, guard on `SEAMLY_SIGNING_*` so a local run with no secrets still produces an unsigned MSI (current behavior)
- [ ] CodeSign.1.4 Keep signing the finished `.msi` last (after its contents are signed and packaged), unchanged from the current "Sign the MSI" step in `ci.yml`'s `windows-msi` job, so both the installer and everything it installs are signed
- [ ] CodeSign.1.5 Add a post-sign verification step to `ci.yml`'s `windows-msi` job — `Get-AuthenticodeSignature` on the signed exes and the `.msi` (Status `Valid`, signer `Seamly Systems, Inc.`, timestamp present) — so a silently-failed or skipped sign is caught in CI before the artifact ships. No such step exists today; the NSIS-era "Print installer signature" step went with the `windows` job
- [ ] CodeSign.1.6 Apply the same inner-exe signing to the arm64 MSI leg (`seamly2d.exe` + `seamlyme.exe` + `seamlayout.exe`)
- [x] CodeSign.1.7 - Inner-exe signing is covered for both architectures by CodeSign.1.2/1.3/1.6
- [ ] CodeSign.1.8 Update the docs: `.github/workflows/CODE_SIGNING.md`, `scripts/packaging/windows/README.md` (Code signing section) and `README_WINDOWS_BUILD.md` §5 ("CI equivalent" — renumbered from §6 on 2026-07-29 when the historical problem sections were folded away) to state that the exes/DLLs *and* the `.msi` are signed, how the signing is guarded on the secrets, and how it is verified
- [ ] CodeSign.1.9 Verify on a clean Windows machine: the installer UAC prompt shows "Verified publisher: Seamly Systems, Inc.", no SmartScreen "unrecognized app" prompt appears, and each installed app reports a `Valid`, timestamped Authenticode signature (`Get-AuthenticodeSignature`)
