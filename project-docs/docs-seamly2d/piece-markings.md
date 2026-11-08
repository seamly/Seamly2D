# Piece markings

Piece Properties provides separate Drill Holes and Reference Lines pages. Markings use live piece anchors, so they follow edited and graded geometry.

Piece Mode uses distinct symbols:

- Drill holes appear as blue unfilled circles with black outlines.
- Stripe references use parallel blue lines.
- Plaid references use intersecting blue lines.
- Live anchors appear as blue point markers with black outlines.

Seamly2D prevents deletion of an anchor used by a drill hole or a legacy reference line.
Remove the marking before deleting its anchor.

New reference lines use two distinct main-path node IDs. Files saved by earlier versions may use two live anchor IDs.
Those lines remain loadable, and their anchors cannot be deleted until the lines are removed.
A line with one node ID and one anchor ID is invalid.
