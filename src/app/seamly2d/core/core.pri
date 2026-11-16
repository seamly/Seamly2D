# ADD TO EACH PATH $$PWD VARIABLE!!!!!!
# This need for correct working file translations.pro

HEADERS += \
    $$PWD/application_2d.h \
    $$PWD/command_line.h \
    $$PWD/vformulaproperty.h \
    $$PWD/vformulapropertyeditor.h \
    $$PWD/vtooloptionspropertybrowser.h

SOURCES += \
    $$PWD/application_2d.cpp \
    $$PWD/command_line.cpp \
    $$PWD/vformulaproperty.cpp \
    $$PWD/vformulapropertyeditor.cpp \
    $$PWD/vtooloptionspropertybrowser.cpp

# Windows only: SeamlyLayout opens in front of Seamly2D.
win32 {
    HEADERS += $$PWD/windows_foreground_window.h
    SOURCES += $$PWD/windows_foreground_window.cpp
    # Qt 6 does not link user32 for the app; the Win32 window calls need it.
    LIBS += -luser32
}

macx{
    HEADERS += $$PWD/macos_dock.h
    OBJECTIVE_SOURCES += $$PWD/macos_dock.mm
    LIBS += -framework AppKit
}
