<h1>Calling Seamly2D from the command or terminal window</h1>
<div style="text-align:left;margin-left:0in;margin-right:0in;">
<p style="font-size:150%;">
Usage:<br/>
<b>seamly2d [options] filename</b>
</p>
<br/>
{| style="border-spacing:0;margin:auto;width:80%;"
|-
| style="background-color:#000080;border-top:0.05pt solid #000000;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | <b>Arguments:</b>
| style="background-color:#000080;border:0.05pt solid #000000;padding:0.0382in;color:#ffffff;" | <b>Definition:</b>
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | filename
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Pattern filename
|-
|}
<br/>
{| style="border-spacing:0;margin:auto;width:80%;"
|-
! style="width: 20%;background-color:#000080;border-top:0.05pt solid #000000;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | <b>Options:</b>
! style="background-color:#000080;border:0.05pt solid #000000;padding:0.0382in;color:#ffffff;" | <b>Definition:</b>
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -h, --help
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Displays this help.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -v, --version
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Displays version information.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -b, --basename <base layout filename>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | The base filename of exported layout files. Use it to enable console export mode.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -d, --destination <destination folder>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | The path to output destination folder. By default the directory at which the application was started.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -m, --mfile <measure file>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Path to custom measure file (export mode).
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -f, --format <format number>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;" | <div style="color:#000000;">Number corresponding to output format (default = 0, export mode):</div>

<div style="color:#000000;"><nowiki>* Svg files (*.svg) = 0,</nowiki></div>

<div style="color:#000000;"><nowiki>* PDF files (*.pdf) = 1,</nowiki></div>

<div style="color:#000000;"><nowiki>* Image files (*.png) = 2,</nowiki></div>

<div style="color:#000000;"><nowiki>* Wavefront OBJ (*.obj) = 3,</nowiki></div>

<div style="color:#000000;"><nowiki>* PS files (*.ps) = 4,</nowiki></div>

<div style="color:#000000;"><nowiki>* EPS files (*.eps) = 5,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF R10 (flat) files (*.dxf) = 6,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF R11/12 (flat) files (*.dxf) = 7,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF R13 (flat) files (*.dxf) = 8,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF R14 (flat) files (*.dxf) = 9,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF 2000 (flat) files (*.dxf) = 10,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF 2004 (flat) files (*.dxf) = 11,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF 2007 (flat) files (*.dxf) = 12,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF 2010 (flat) files (*.dxf) = 13,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF 2013 (flat) files (*.dxf) = 14,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF R10 AAMA files (*.dxf) = 15,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF R11/12 AAMA files (*.dxf) = 16,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF R13 AAMA files (*.dxf) = 17,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF R14 AAMA files (*.dxf) = 18,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF 2000 AAMA files (*.dxf) = 19,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF 2004 AAMA files (*.dxf) = 20,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF 2007 AAMA files (*.dxf) = 21,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF 2010 AAMA files (*.dxf) = 22,</nowiki></div>

<div style="color:#000000;"><nowiki>* AutoCAD DXF 2013 AAMA files (*.dxf) = 23,</nowiki></div>

<div style="color:#000000;"><nowiki>* PDF tiled files (*.pdf) = 33.</nowiki></div>
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --bdxf
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Export dxf in binary form.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --text2paths
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Export text as paths.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --exportOnlyDetails
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Export only details. Export details as they positioned in the details mode. Any layout related options will be ignored.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --exportSuchDetails <name regex>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Export only details that match a piece name regex.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -x, --gsize <size value>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;" | <div style="color:#000000;">Set size value a pattern file, that was opened with multisize measurements (export mode). </div>

<div style="color:#000000;">Valid values: 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72cm.</div>
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -e, --gheight <The height value>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;" | <div style="color:#000000;">Set height value a pattern file, that was opened with multisize measurements (export mode). </div>

<div style="color:#000000;">Valid values: 50, 56, 62, 68, 74, 80, 86, 92, 98, 104, 110, 116, 122, 128, 134, 140, 146, 152, 158, 164, 170, 176, 182, 188, 194, 200cm.</div>
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -p, --pageformat <Template number>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;" | <div style="color:#000000;">Number corresponding to layout page template (default = 0, export mode):</div>

<div style="color:#000000;"><nowiki>* A0 = 0,</nowiki></div>

<div style="color:#000000;"><nowiki>* A1 = 1,</nowiki></div>

<div style="color:#000000;"><nowiki>* A2 = 2,</nowiki></div>

<div style="color:#000000;"><nowiki>* A3 = 3,</nowiki></div>

<div style="color:#000000;"><nowiki>* A4 = 4,</nowiki></div>

<div style="color:#000000;"><nowiki>* Letter = 5,</nowiki></div>

<div style="color:#000000;"><nowiki>* Legal = 6,</nowiki></div>

<div style="color:#000000;"><nowiki>* Roll 24in = 7,</nowiki></div>

<div style="color:#000000;"><nowiki>* Roll 30in = 8,</nowiki></div>

<div style="color:#000000;"><nowiki>* Roll 36in = 9,</nowiki></div>

<div style="color:#000000;"><nowiki>* Roll 42in = 10,</nowiki></div>

<div style="color:#000000;"><nowiki>* Roll 44in = 11</nowiki></div>
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -W, --pagew <The page width>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Page width in current units like 12.0 (cannot be used with "pageformat", export mode).
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -H, --pageh <The page height>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Page height in current units like 12.0 (cannot be used with "pageformat", export mode).
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -U, --pageunits <The measure unit>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Page measure units (export mode). Valid values: mm, cm, inch, px.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -i, --ignoremargins
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Ignore printer margins (export mode). Use if need full paper space. In case of later printing you must account for the margins himself.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -L, --lmargin <The left margin>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Page left margin in current units like 3.0 (export mode). If not set will be used value from default printer. Or 0 if none printers was found.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -R, --rmargin <The right margin>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Page right margin in current units like 3.0 (export mode). If not set will be used value from default printer. Or 0 if none printers was found.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -T, --tmargin <The top margin>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Page top margin in current units like 3.0 (export mode). If not set will be used value from default printer. Or 0 if none printers was found.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -B, --bmargin <The bottom margin>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Page bottom margin in current units like 3.0 (export mode). If not set will be used value from default printer. Or 0 if none printers was found.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -r, --rotate <Angle>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;" | <div style="color:#000000;">Rotation in degrees (one of predefined, export mode). </div>

<div style="color:#000000;">Default value is 180. 0 is no-rotate. </div>

<div style="color:#000000;">Valid values: 1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 15, 18, 20, 24, 30, 36, 40, 45, 60, 72, 90, 180. </div>

<div style="color:#000000;">Each value show how many times details will be rotated. For example 180 mean two times (360/180=2) by 180 degree.</div>
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -c, --crop
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Auto crop unused length (export mode).
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -u, --unite
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Unite pages if possible (export mode). Maximum value limited by QImage that supports only a maximum of 32768x32768 px images.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -S, --savelen
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Save length of the sheet if set (export mode). The option tells the program to use as much as possible width of sheet. Quality of a layout can be worse when this option was used.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -l, --layounits <The unit>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Layout units (as paper's one except px, export mode).
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -s, --shiftlen <Shift/Offset length>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Shift/Offset layout length measured in layout units (export mode). The option show how many points along edge will be used in creating a layout.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -G, --gapwidth <The gap width>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | The layout gap width x2, measured in layout units (export mode). Set distance between details and a detail and a sheet.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -g, --groups <Grouping type>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;" | <div style="color:#000000;">Sets layout groupping cases (export mode): </div>

<div style="color:#000000;">Three groups: big, middle, small = 0;</div>

<div style="color:#000000;">Two groups: big, small = 1;</div>

<div style="color:#000000;">Descending area = 2.</div>
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | -t, --test
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Run the program in a test mode. The program in this mode loads a single pattern file and silently quit without showing the main window. The key have priority before key 'basename'.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --no-scaling
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Disable high dpi scaling. Call this option if has problem with scaling (by default scaling enabled). Alternatively you can use the QT_AUTO_SCREEN_SCALE_FACTOR=0 environment variable.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --csvWithHeader
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Export to csv with header. By default disabled.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --csvCodec <Codec name>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;" | <div style="color:#000000;">Specify codec that will be used to save data. List of supported codecsprovided by Qt. Default value depend from system. On Windows, the codec will be based on a system locale. On Unix systems, the codec will might fall back to using the iconv library if no builtin codec for the locale can be found. </div>

<div style="color:#000000;">Valid values for this installation:</div>

<div style="color:#000000;"><nowiki>* US-ASCII,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-1,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-2,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-3,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-4,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-5,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-6,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-7,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-8,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-9,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-10,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-2022-JP-1,</nowiki></div>

<div style="color:#000000;"><nowiki>* Shift_JIS,</nowiki></div>

<div style="color:#000000;"><nowiki>* EUC-JP,</nowiki></div>

<div style="color:#000000;"><nowiki>* US-ASCII,</nowiki></div>

<div style="color:#000000;"><nowiki>* windows-949,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-2022-KR,</nowiki></div>

<div style="color:#000000;"><nowiki>* windows-949,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-2022-JP,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-2022-JP-2,</nowiki></div>

<div style="color:#000000;"><nowiki>* GBK,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-6,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-6,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-8,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-8,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-2022-CN,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-2022-CN-EXT,</nowiki></div>

<div style="color:#000000;"><nowiki>* UTF-8,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-13,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-14,</nowiki></div>

<div style="color:#000000;"><nowiki>* ISO-8859-15,</nowiki></div>

<div style="color:#000000;"><nowiki>* GBK,</nowiki></div>

<div style="color:#000000;"><nowiki>* GB18030,</nowiki></div>

<div style="color:#000000;"><nowiki>* UTF-16,</nowiki></div>

<div style="color:#000000;"><nowiki>* UTF-32,</nowiki></div>

<div style="color:#000000;"><nowiki>* SCSU,</nowiki></div>

<div style="color:#000000;"><nowiki>* UTF-7,</nowiki></div>

<div style="color:#000000;"><nowiki>* UTF-16BE,</nowiki></div>

<div style="color:#000000;"><nowiki>* UTF-16LE,</nowiki></div>

<div style="color:#000000;"><nowiki>* UTF-16,</nowiki></div>

<div style="color:#000000;"><nowiki>* CESU-8,</nowiki></div>

<div style="color:#000000;"><nowiki>* UTF-32,</nowiki></div>

<div style="color:#000000;"><nowiki>* UTF-32BE,</nowiki></div>

<div style="color:#000000;"><nowiki>* UTF-32LE,</nowiki></div>

<div style="color:#000000;"><nowiki>* BOCU-1,</nowiki></div>

<div style="color:#000000;"><nowiki>* hp-roman8,</nowiki></div>

<div style="color:#000000;"><nowiki>* Adobe-Standard-Encoding,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM850,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM862,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM-Thai,</nowiki></div>

<div style="color:#000000;"><nowiki>* Shift_JIS,</nowiki></div>

<div style="color:#000000;"><nowiki>* GBK,</nowiki></div>

<div style="color:#000000;"><nowiki>* Big5,</nowiki></div>

<div style="color:#000000;"><nowiki>* macintosh,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM037,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM273,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM277,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM278,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM280,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM284,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM285,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM290,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM297,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM420,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM424,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM437,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM500,</nowiki></div>

<div style="color:#000000;"><nowiki>* cp851,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM852,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM855,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM857,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM860,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM861,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM863,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM864,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM865,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM868,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM869,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM870,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM871,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM918,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM1026,</nowiki></div>

<div style="color:#000000;"><nowiki>* KOI8-R,</nowiki></div>

<div style="color:#000000;"><nowiki>* HZ-GB-2312,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM866,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM775,</nowiki></div>

<div style="color:#000000;"><nowiki>* KOI8-U,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM00858,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM01140,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM01141,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM01142,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM01143,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM01144,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM01145,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM01146,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM01147,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM01148,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM01149,</nowiki></div>

<div style="color:#000000;"><nowiki>* Big5-HKSCS,</nowiki></div>

<div style="color:#000000;"><nowiki>* IBM1047,</nowiki></div>

<div style="color:#000000;"><nowiki>* windows-1250,</nowiki></div>

<div style="color:#000000;"><nowiki>* windows-1251,</nowiki></div>

<div style="color:#000000;"><nowiki>* windows-1252,</nowiki></div>

<div style="color:#000000;"><nowiki>* windows-1253,</nowiki></div>

<div style="color:#000000;"><nowiki>* windows-1254,</nowiki></div>

<div style="color:#000000;"><nowiki>* windows-1255,</nowiki></div>

<div style="color:#000000;"><nowiki>* windows-1256,</nowiki></div>

<div style="color:#000000;"><nowiki>* windows-1257,</nowiki></div>

<div style="color:#000000;"><nowiki>* windows-1258,</nowiki></div>

<div style="color:#000000;"><nowiki>* TIS-620,</nowiki></div>

<div style="color:#000000;"><nowiki>* TSCII.</nowiki></div>
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --csvSeparator <Separator character>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;" | <div style="color:#000000;">Specify csv separator character. Default value is ','. Valid characters:</div>

<div style="color:#000000;"><nowiki>* 'Tab',</nowiki></div>

<div style="color:#000000;"><nowiki>* ';' </nowiki></div>

<div style="color:#000000;"><nowiki>* 'Space',</nowiki></div>

<div style="color:#000000;"><nowiki>* ','.</nowiki></div>
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --csvExportFM <Path to csv file>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Calling this command enable exporting final measurements. Specify path to csv file with final measurements. The path must contain path to directory and name of file. It can be absolute or relatetive. In case of relative path will be used current working directory to calc a destination path.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --tiledPageformat <Template number>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;" | <div style="color:#000000;">Number corresponding to tiled pdf page template (default = 0, export mode with tiled pdf format):</div>

<div style="color:#000000;"><nowiki>* A0 = 0,</nowiki></div>

<div style="color:#000000;"><nowiki>* A1 = 1,</nowiki></div>

<div style="color:#000000;"><nowiki>* A2 = 2,</nowiki></div>

<div style="color:#000000;"><nowiki>* A3 = 3,</nowiki></div>

<div style="color:#000000;"><nowiki>* A4 = 4,</nowiki></div>

<div style="color:#000000;"><nowiki>* Letter = 5,</nowiki></div>

<div style="color:#000000;"><nowiki>* Legal = 6.</nowiki></div>
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --tiledlmargin <The left margin>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Tiled page left margin in current units like 3.0 (export mode). If not set will be used default value 1 cm.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --tiledrmargin <The right margin>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Tiled page right margin in current units like 3.0 (export mode). If not set will be used default value 1 cm.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --tiledtmargin <The top margin>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Tiled page top margin in current units like 3.0 (export mode). If not set will be used value default value 1 cm.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --tiledbmargin <The bottom margin>
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Tiled page bottom margin in current units like 3.0 (export mode). If not set will be used value default value 1 cm.
|-
| style="background-color:#4d4d4d;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:none;padding:0.0382in;color:#ffffff;" | --tiledLandscape
| style="background-color:#dfdfdf;border-top:none;border-bottom:0.05pt solid #000000;border-left:0.05pt solid #000000;border-right:0.05pt solid #000000;padding:0.0382in;color:#000000;" | Set tiled page orientation to landscape (export mode). Default value if not set portrait.
|-
|}
