message(" ")
message("===== Entering Seamly.pro =====")
message(" ")

#Check if spaces are in directory names
LIST = $$split(PWD,' ')
count(LIST, 1, >): error("The build will fail. Path '$${PWD}' contains space!!!")

LIST = $$split(OUT_PWD,' ')
count(LIST, 1, >): error("The build will fail. Path '$${OUT_PWD}' contains space!!!")

TEMPLATE = subdirs

# SUBDIRS lists only the two direct children of this project: the source
# tree and the macOS packaging step. seamly2d and seamlyme are NOT listed
# here; they sit two levels deeper (src/src.pro -> app -> app.pro ->
# seamly2d, seamlyme) so that libs builds before app, and app's own
# seamly2d.depends = seamlyme (src/app/app.pro) stays local to that level.
# packaging_macos depends on the whole src subtree, not on one app, so it
# belongs as a peer of src here rather than a peer of seamly2d/seamlyme.
SUBDIRS = \
    src \
    packaging_macos

# qmake variable names cannot contain "/", so packaging/macos needs an alias
# here for .depends to register. A literal "packaging/macos.depends" line is
# silently invalid and lets a parallel `make -j` build the DMG before src/
# finishes, racing the packaging/macos.pro hdiutil step against Seamly2D.app.
packaging_macos.subdir = packaging/macos
packaging_macos.depends = src

qtPrepareTool(LUPDATE, lupdate)
lupdate.commands = $$LUPDATE -noobsolete -locations none $$shell_path($${PWD}/share/translations/translations.pro)
lupdate.commands += && $$LUPDATE -noobsolete $$shell_path($${PWD}/share/translations/measurements.pro)

QMAKE_EXTRA_TARGETS += lupdate
