# Future TODO's

Add tasks in project-docs/TODO_MIGRATE.md -->

1. Create a plan to merge `src\app\seamlylayout\.claude` and `src\app\seamlylayout\.cgithub` data into the project `.claude` and `.github` data.

Add task in project-docs/TODO_SEAMLY2D.md -->

* Add a tool for each capability:

  * ReNumber IDs - This tool will be in the Piece Tool Group in Draft Mode's left column. Use an svg processing library (not regex) to process the pattern:
    * use case: when a user adds and deletes many pattern objects in a piece in Draft mode, the id values become higher than the number of elements in the pattern;
    * prompt the user with confirmation message that all IDs will be renumbered in the pattern, continue / cancel, if user selects cancel then return else continue;
    * get all elements in the selected piece into an elements object
    * for each element in the elements object:
      * store unique integer values from attributes in {id attributes list} and it's tag value and its attribute name to IDS_UNIQUE[(tag, attribute, value)], keep a count the unique attribute values in ID_COUNT, store ID_COUNT to ID_COUNTDOWN, store 0 to ID_COUNTUP, and sort ID_VALUES in descending order on value to ID_VALUES_SORTED.
      // Note: for tag=="item" the attribute will be blank because the id is the content of the item element.
      endfor
    * while ID_COUNTUP < ID_COUNT:
      * store ID_VALUES_SORTED[ID_COUNTUP] to tag, attribute, id_value // index 0 holds highest id value
      * find element el where el.tag == tag, el.attribute.name == attribute, and ( if exists el.content then el.content == id_value else el.attribute.value = id_value)
      * if found:
        * replace el.attribute.id_value or el.content with "\<ID_COUNTDOWN>" // max id value that has not been used
        else:
        * error
        endif
      * increment ID_COUNTUP, decrement ID_COUNTDOWN
      endwhile
    * reparse pattern and display

  * ReNumber Automatic Point Names - This tool will be in the Piece Tool Group in Draft Mode's left column. Use an svg processing library (not regex) to process the pattern:
    * use case: when many points with automatically generated names are added and deleted, the integer values become higher than the number of automatically generated points in the pattern and might not reflect the order they were created, the letter may need to be reassigned
    * Note: the automatically created Point names start with the piece letter followed by an integer and do not contain underscores "_".
    * When the user clicks on the tool they immediately select a piece to open the dialog that displays the name and letter of the selected piece and prompts the user to enter a new letter for the selected piece (optional)
    * if new letter is entered:
      * validate new letter against the letter of all other pieces -- if error generate msg with OK prompt and close else continue / cancel, if user presses cancel then return else continue
      * store new letter to NEW_LETTER, store piece letter to PIECE_LETTER
      else:
      * store piece letter to NEW_LETTER and to PIECE_LETTER
      endif
    * store piece name to PIECE_NAME
    * store all draftBlock names that exist below PIECE_NAME in the pattern in order to DRAFTBLOCK_NAMES[]
    * store all elements in the selected piece into an elements object
    * For each element el in elements object:
      * if el.tag in {point element list}:
        * for each el.attribute.name in {name attributes list}:
          * store el.attribute.name to OLD_NAME
          * if el.attribute.value ((does not contain underscore "_") and (contains PIECE_LETTER followed by an integer char)):
            * increment NAME_COUNT, store NAME_COUNT to NAME_COUNTDOWN, store 0 to NAME_COUNTUP, and store el.tag, el.id, el.attribute.name, el.attribute.value to NAME_VALUES[(tag, id, attribute, value)], and store NAME_VALUES[] in descending order of value to NAME_VALUES_SORTED[]
            * while NAME_COUNTUP < NAME_COUNT:
              * store  "<NEW_LETTER><NAME_COUNTDOWN>" to NEW_NAME
              * store NAME_VALUES_SORTED[ID_COUNTUP] to tag, id, attribute, point_integer_value // index 0 holds highest point integer value
              * find element el where el.tag == tag, el.id=id
              * if found:
                * if (exist el.attribute.name) and (el.attribute.value==point_integer_value):
                  * replace el.attribute.value with NEW_NAME
                  else:
                  * error
                  endif
                  * for each element el_below that is below the current el element to the end of elements object:
                    * if (el_below.attribute.value contains ("_\<OLD_NAME>" or "\<OLD_NAME>\_"))
                      * replace el_below.attribute.value with NEW_NAME
                      * for each draftblock in DRAFTBLOCK_NAMES[]:
                        * store the draftblock into an elements object
                        * for each element draft_el in draftblock:
                          * for each attrib in draft_el.attribs:
                            * for key, value in attrib:
                              * if value contains "_\<OLD_NAME>" or "\<NEW_NAME>\_":
                                * replace OLD_NAME in value with NEW_NAME
                                endif
                              endfor
                            endfor
                          endfor
                        endfor
                      endif
                    endfor
                  endif
                endif
                * decrement NAME_COUNTDOWN, increment NAME_COUNTUP
              endwhile
            endif
          endfor
        endif
      endfor
    * reparse pattern and update display.


Add task in project-docs/TODO_SEAMLYLAYOUT.md -->

Add rule to CLAUDE.md -->

1. Adhere to code style rules as defined in `.github\README-CODE-STYLES.md`

Ask Claude -->
