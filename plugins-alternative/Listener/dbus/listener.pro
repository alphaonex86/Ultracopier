CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

TEMPLATE        = lib
CONFIG		+= plugin
QT		+= network dbus
HEADERS         = \
    $$PWD/listener.h \
    $$PWD/Environment.h \
    $$PWD/Variable.h \
    $$PWD/DebugEngineMacro.h \
    $$PWD/StructEnumDefinition.h \
    $$PWD/../../../interface/PluginInterface_Listener.h \
    $$PWD/Catchcopy.h
SOURCES         = \
    $$PWD/listener.cpp \
    $$PWD/Catchcopy.cpp
TARGET          = $$qtLibraryTarget(listener)
