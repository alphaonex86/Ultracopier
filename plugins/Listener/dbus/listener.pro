TEMPLATE        = lib
CONFIG		+= plugin qdbus
QT		+= network
HEADERS         = listener.h \
    Environment.h \
    Variable.h \
    DebugEngineMacro.h \
    StructEnumDefinition.h \
    ../../../interface/PluginInterface_Listener.h \
    Catchcopy.h
SOURCES         = listener.cpp \
    Catchcopy.cpp
TARGET          = $$qtLibraryTarget(listener)

