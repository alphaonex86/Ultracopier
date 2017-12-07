CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

TEMPLATE        = lib
CONFIG		+= plugin
QT		+= network
win32:LIBS += -ladvapi32
HEADERS         = \
    $$PWD/listener.h \
    $$PWD/catchcopy-api-0002/VariablesCatchcopy.h \
    $$PWD/catchcopy-api-0002/ServerCatchcopy.h \
    $$PWD/catchcopy-api-0002/ExtraSocketCatchcopy.h \
    $$PWD/Environment.h \
    $$PWD/Variable.h \
    $$PWD/DebugEngineMacro.h \
    $$PWD/StructEnumDefinition.h \
    $$PWD/../../../interface/PluginInterface_Listener.h \
    $$PWD/../../../cpp11addition.h
SOURCES         = \
    $$PWD/listener.cpp \
    $$PWD/catchcopy-api-0002/ServerCatchcopy.cpp \
    $$PWD/catchcopy-api-0002/ExtraSocketCatchcopy.cpp \
    $$PWD/../../../cpp11addition.cpp \
    $$PWD/../../../cpp11additionstringtointcpp.cpp
TARGET          = $$qtLibraryTarget(listener)
