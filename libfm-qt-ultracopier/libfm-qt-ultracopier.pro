CONFIG += c++11

TEMPLATE        = lib
QT		+= network widgets
TARGET          = $$qtLibraryTarget(fm-qt)
LIBS += -Llibfm-qt
INCLUDEPATH += /usr/include/glib-2.0/ /usr/lib64/glib-2.0/include/
DEFINES += QT_NO_KEYWORDS

HEADERS += \
    utilities.h

SOURCES += \
    utilities.cpp
