# ADD TO EACH PATH $$PWD VARIABLE!!!!!!
# This need for correct working file translations.pro

HEADERS += \
    $$PWD/application_2d.h \
    $$PWD/vformulaproperty.h \
    $$PWD/vformulapropertyeditor.h \
    $$PWD/vtooloptionspropertybrowser.h \
    $$PWD/vcmdexport.h

SOURCES += \
    $$PWD/application_2d.cpp \
    $$PWD/vformulaproperty.cpp \
    $$PWD/vformulapropertyeditor.cpp \
    $$PWD/vtooloptionspropertybrowser.cpp \
    $$PWD/vcmdexport.cpp

# Windows only: SeamlyLayout opens in front of Seamly2D.
win32 {
    HEADERS += $$PWD/windows_foreground_window.h
    SOURCES += $$PWD/windows_foreground_window.cpp
}
