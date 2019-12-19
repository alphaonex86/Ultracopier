CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

TEMPLATE        = lib
CONFIG		+= plugin
QT		+= network widgets
DEFINES         += _GNU_SOURCE
win32:LIBS += -ladvapi32
TARGET          = $$qtLibraryTarget(libfm-qt-ultracopier)
LIBS += libfm-qt
INCLUDEPATH += /usr/include/glib-2.0/ /usr/lib64/glib-2.0/include/

HEADERS += \
    utilities.h

SOURCES += \
    utilities.cpp
