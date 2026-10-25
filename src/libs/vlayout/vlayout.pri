# ADD TO EACH PATH $$PWD VARIABLE!!!!!!
# This need for corect working file translations.pro

HEADERS += \
    $$PWD/stable.h \
    $$PWD/vlayoutdef.h \
    $$PWD/vgraphicsfillitem.h \
    $$PWD/vlayoutpiece.h \
    $$PWD/vlayoutpiece_p.h \
    $$PWD/vlayoutpiecepath.h \
    $$PWD/vlayoutpiecepath_p.h \
    $$PWD/svg_text_item.h

SOURCES += \
    $$PWD/vgraphicsfillitem.cpp \
    $$PWD/vlayoutpiece.cpp \
    $$PWD/vlayoutpiecepath.cpp \
    $$PWD/svg_text_item.cpp

*msvc*:SOURCES += $$PWD/stable.cpp
