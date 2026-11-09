# Label filename variables

`%pFileName%` and `%mFileName%` include the file extension. They omit the directory path.
`%pExt%` and `%mExt%` still provide the extension without the dot.

Pattern labels, piece labels, and the label editor preview use the same filename behavior.
Legacy pattern conversion uses `%mFileName%` alone to avoid appending the extension twice.

Existing templates that combine a filename variable with its extension variable, such as
`%mFileName%.%mExt%`, display the extension twice. Remove the extension variable and dot
from those templates.
