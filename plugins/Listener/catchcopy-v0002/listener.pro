CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

TEMPLATE        = lib
CONFIG		+= plugin
QT		+= network
win32:LIBS += -ladvapi32
HEADERS         = listener.h \
    catchcopy-api-0002/VariablesCatchcopy.h \
    catchcopy-api-0002/ServerCatchcopy.h \
    catchcopy-api-0002/ExtraSocketCatchcopy.h \
    Environment.h \
    Variable.h \
    DebugEngineMacro.h \
    StructEnumDefinition.h \
    ../../../interface/PluginInterface_Listener.h
SOURCES         = listener.cpp \
    catchcopy-api-0002/ServerCatchcopy.cpp \
    catchcopy-api-0002/ExtraSocketCatchcopy.cpp
TARGET          = $$qtLibraryTarget(listener)
