CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

QT += widgets xml
DEFINES += UNICODE _UNICODE
TEMPLATE        = lib
CONFIG         += plugin

HEADERS         += \
    $$PWD/../../../interface/PluginInterface_CopyEngine.h \
    $$PWD/../../../interface/OptionInterface.h \
    $$PWD/../../../interface/FacilityInterface.h \
    $$PWD/../../../cpp11addition.h \
    $$PWD/CopyEngine.h \
    $$PWD/CopyEngineFactory.h
SOURCES         += \
    $$PWD/../../../cpp11addition.cpp \
    $$PWD/../../../cpp11additionstringtointcpp.cpp \
    $$PWD/CopyEngine.cpp \
    $$PWD/CopyEngineFactory.cpp
TARGET          = $$qtLibraryTarget(copyEngine)

win32 {
    LIBS += -ladvapi32
}
