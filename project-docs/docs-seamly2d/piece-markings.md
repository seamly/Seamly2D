# Piece markings

Piece Properties provides separate Drill Holes and Reference Lines pages.
Drill holes use live piece anchors. Stripe and Plaid references use main-path point nodes.
Both follow edited and graded geometry.

Piece Mode uses distinct symbols:

- Drill holes appear as blue unfilled circles with black outlines.
- Stripe references use parallel blue lines.
- Plaid references use intersecting blue lines.
- Live anchors appear as blue point markers with black outlines.

Seamly2D prevents deletion of an anchor used by a drill hole or a legacy reference line.
Remove the marking before deleting its anchor.

Adding an anchor creates one undo operation, including its modeling node and piece membership.
Undo enables **Redo add anchor point** in the Edit menu and toolbar.
Redo restores the same anchor ID. Repeated undo/redo does not leave detached modeling nodes.

New reference lines use two distinct main-path node IDs. Files saved by earlier versions may use two live anchor IDs.
Those lines remain loadable, and their anchors cannot be deleted until the lines are removed.
A line with one node ID and one anchor ID is invalid.

## Reference endpoints

The Reference Lines page offers **Start path node** and **End path node**, even when the piece has no anchors.
Adding or removing anchors does not change those choices. Curve IDs are not point endpoints.
Select two distinct path nodes, then select **Stripe** or **Plaid** and **Add**.
The saved line contains those path-node IDs, not matching anchor IDs.
To replace a legacy anchor-based reference, remove it and add a reference between path nodes.

The `pieceDialogReferencesUsePathNodes` regression covers page changes, both reference types, serialization,
anchor independence, and endpoint changes after editing path geometry.
