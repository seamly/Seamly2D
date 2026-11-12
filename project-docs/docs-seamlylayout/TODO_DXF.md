# TODO — DXF-ASTM compliance

Fixes from the aw.fyi (ApparelWerks) ASTM D6673 reports on DXF-ASTM (R13) and DXF-ASTM (CLO3D) exports.

- Task prefix: `DXF.`
- Check off each subtask when it is done. Move a finished task to `project-docs/TODO_COMPLETED.md`.
- Rules and report results: `dxf-docs/seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md` ("aw.fyi results").

## [ ] Task DXF.18 — Notches on a piece with a built-in seam allowance

On a piece with a built-in seam allowance, Seamly2D's main path is the cut line. Seamly2D sends the seam allowance width at each notch in `data-seam-allowances`. SeamlyLayout makes each layer 4 slit half that width. Before this task, SeamlyLayout had no width there, so each slit kept its drawn depth.

- [x] DXF.18.1 `converter::notch_polylines`: read `data-seam-allowances`, one width per notch subpath; a list of the wrong length is ignored
- [x] DXF.18.2 `astm_notch::build_notches`: Seamly2D's width first, then the measured width, then the drawn depth. Slit depth = half the width
- [x] DXF.18.3 Tests: `seamly2d_width_sets_slit_depth_before_the_measured_width`, `built_in_seam_allowance_has_boundary_and_sew_line` (was `built_in_seam_allowance_notches_use_seamly2d_widths`)
- [x] DXF.18.4 Compliance doc "Notch rules"
- [ ] DXF.18.5 Next MSI: user check. Seamly2D.10.6 points to this check.
  - Steps: in Seamly2D, add a notch to a piece with a built-in seam allowance. In `male_shirt`, CollarTopInterface-Built-in-SA has no notch now. Press Layout. In SeamlyLayout, export DXF-ASTM (R13).
  - Expected: each layer 4 slit on that piece is half the seam allowance width. A 10 mm seam allowance gives a 5 mm slit.
