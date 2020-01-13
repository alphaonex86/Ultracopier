CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

TEMPLATE        = lib
CONFIG         += plugin
LIBS += -ladvapi32
HEADERS         = \
    $$PWD/sessionLoader.h \
    $$PWD/StructEnumDefinition.h \
    $$PWD/SessionLoaderWindowsVariable.h \
    $$PWD/Environment.h \
    $$PWD/DebugEngineMacro.h \
    $$PWD/../../../interface/PluginInterface_SessionLoader.h
SOURCES         = sessionLoader.cpp
TARGET          = $$qtLibraryTarget(sessionLoader)
