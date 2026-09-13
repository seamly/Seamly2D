message(" ")
message("===== Entering Seamly.pro =====")
message(" ")

# ===== Check if spaces are in directory names =====
LIST = $$split(PWD,' ')
count(LIST, 1, >): error("The build will fail. Path '$${PWD}' contains space!!!")

# ===== Guard against spaces in the build output path. =====
LIST = $$split(OUT_PWD,' ')
count(LIST, 1, >): error("The build will fail. Path '$${OUT_PWD}' contains space!!!")

# ===== SUBDIRS =====
# - `TEMPLATE = subdirs` requires each SUBDIRS entry to contain its own .pro file, so no
# additional top-level .pro file is required.
#
# SUBDIRS lists the two direct children of this project: 
# 1. the source tree `src` (utilized by all builds, windows/linux/macos).  
# 2. the macOS packaging tree `packaging_macos`, required only by the macos build. 
#
# - The windows build DOES NOT use qmake -- it uses powershell scripts (.ps1) outside of qmake,
# that utilize WIX configuration files (.wsx) to build an MSI each for x64 and arm64 archictectures 
# in the `packaging/windows` tree. No SUBDIRS entry needed for windows MSI packaging; see ci.yml, 
# 'Windows: Build MSI (${{ matrix.arch }})' job.
#
# - The linux build DOES use qmake -- `qmake -config release` + `make` builds Seamly2D
# directly from `Seamly.pro` at project root (no separate packaging subdir required), then
# `make install` populates `AppDir/`, which the external `linuxdeploy-x86_64.AppImage` tool 
# wraps into the .AppImage file; see ci.yml, 'Linux: Build AppImage' job. 
# No SUBDIRS entry needed for linux APPIMAGE packaging.
#
# - FlatPak packaging is not yet implemented (see TODO_INSTALLER_LINUX_FLATPAK.md); see ci.yml,
# No SUBDIRS entry needed for linux FLATPAK packaging.
#
# - The macos build DOES use qmake -- Seamly2D.dmg, SeamlyME.dmg, and SeamlyLayout.dmg are built
# using `hdiutil` via qmake rules defined in `packaging/macos/macos.pro`. The `packaging_macos` tree
# is a peer of `src` (not nested inside it) so it can set `.depends = src` and will run only after the
# whole src tree finishes building.
TEMPLATE = subdirs
SUBDIRS = \
    src \
    packaging_macos

# ===== MacOS packaging =====
# Handle qmake, hdiutil, & racing condition during macos build:
# qmake variable names cannot contain "/", so `packaging/macos` needs an alias
# here for .depends to register. A literal "packaging/macos.depends" line is
# silently invalid which allows a parallel `make -j` step to build the DMG before src/
# finishes, racing the packaging/macos.pro hdiutil step against Seamly2D.app; the hdiutil
# step fails as it runs prior to app file creation. 
packaging_macos.subdir = packaging/macos
packaging_macos.depends = src

# ===== Translation updates =====
qtPrepareTool(LUPDATE, lupdate)
lupdate.commands = $$LUPDATE -noobsolete -locations none $$shell_path($${PWD}/share/translations/translations.pro)
lupdate.commands += && $$LUPDATE -noobsolete $$shell_path($${PWD}/share/translations/measurements.pro)
QMAKE_EXTRA_TARGETS += lupdate
