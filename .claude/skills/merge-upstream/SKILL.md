---
name: merge-upstream
description: Merge upstream FashionFreedom develop into run-seamlyLayout. Use only when Sue asks for an upstream or develop merge.
---

# Merge from Upstream `develop`

1. `git fetch upstream`; fast-forward `develop` to `upstream/develop`; push `origin develop`.
2. `git checkout -b task-merge-develop run-seamlyLayout`.
3. `git merge --no-ff --no-commit develop`.
4. Review `git diff --cached -- src/` for code this branch removed on purpose.
5. Resolve conflicts for this branch's intent, not "theirs".
6. Run `git grep -n "convertPdfToPs\|PDFTOPS"`. Expect no hits.
7. Build with `packaging\windows\local_build_msi.ps1`. A failure stops the merge.
8. Commit, merge `--no-ff` into `run-seamlyLayout`. Sue pushes.
9. Full CI: `gh workflow run ci.yml --ref run-seamlyLayout`.

Enable `git config rerere.enabled true` so Git reuses recorded conflict resolutions.

## Retired code that `develop` still has

- `MainWindowsNoGUI::exportEPS`, `exportPS`, `convertPdfToPs` (pdftops), retired in `14e1d8723b`.
  `develop` restores them whenever it edits nearby lines. Delete them on each merge.
  Upstream keeps them until the endgame PR (user decision). Do not propose an early upstream PR.
- `src/libs/vdxf/` and the console export mode (`--basename`, `--format`), removed by Task Dead.4.
  SeamlyLayout writes DXF-AAMA (`seamlylayout_DXF_AAMA.md`). On the merge that brings
  upstream PR 1783 (label-independent AAMA piece text):
  - keep `src/libs/vdxf/` deleted;
  - drop the `cutting_text` and `getPieceText()` changes in `vlayoutpiece.cpp` and `vlayoutpiece_p.h`: nothing reads them;
  - drop the `aamaCuttingText` test in `tst_seamly2dcommandline.{h,cpp}`;
  - drop `.github/validation/issue-1783/`: evidence for the removed exporter, with third-party PDFs;
  - rewrite `.github/AAMA-INTEROP.md` to describe SeamlyLayout's DXF-AAMA;
  - fix the `.github/README-AUTOMATION.md` example: Seamly2D has no `--basename` or `--format`;
  - keep the `SEAMLY_DATA_DIR` code; SeamlyLayout follows in Task Layout.101.
