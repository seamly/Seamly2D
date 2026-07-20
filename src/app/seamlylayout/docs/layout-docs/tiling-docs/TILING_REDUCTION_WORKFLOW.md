``
After the 'process layout' workflow is completed but before the updated layout_dom is displayed in the right canvas:

fn check_tile_single_piece_overlap(tileid, P):
- determine if P overlaps tile, return true if they overlap else return false

fn reduce_dom_height(dom, reduce_amt):
- reduce id=contentRect 'height' by reduce_amt
- reduce id=background 'height' by reduce_amt
- reduce <svg> height by reduce_amt

fn check_tile_pieces_overlap(tileid):
- for piece P in pieces:
-- overlap = check_overlap(tileid, P)
-- if overlap then return true
- return false

fn check_pieces_in_BottomTileRow(layout_dom):
- tile_list = rectangles in the <tiledRects> group where id contains "Tile<highest_row_number>"
- found = determine if any tile is overlapped by any piece (true or false)
- if found:
resultreturn result

fn clean_BottomTileRow(layout_dom, tile_list):
- tile_list = check_BottomTileRow(layout_dom):
- delete_BottomRow(layout_dom, tile_list)
- row_height = self.tileHeight
- reduce_dom_height(layout_dom, row_height)

fn clean_BottomRollArea(layout_dom):
- space=(contentRect maxY - lowest piece maxY)
- if space > 20:
-- reduce_dom_height(layout_dom, space)

if (media==paper) and ((paperType==tile) or (paperType==roll)):
- if paperType==tile:
-- pieces_in_BottomTileRow=false
-- Loop while !pieces_in_BottomTileRow:
--- piece_in_BottomTileRow=clean_BottomTileRow(layout_dom)

- if paperType==roll:
-- // get highest y-value from pattern pieces as highestY
-- // trimmedHeight = highestY + marginBottomY
-- // set <svg> height=trimmedHeight
