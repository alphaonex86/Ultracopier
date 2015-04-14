TEMPLATE        = lib
CONFIG         += plugin
LIBS += -ladvapi32
HEADERS         = sessionLoader.h \
    StructEnumDefinition.h \
    Variable.h \
    Environment.h \
    DebugEngineMacro.h \
    ../../../interface/PluginInterface_SessionLoader.h
SOURCES         = sessionLoader.cpp
TARGET          = $$qtLibraryTarget(sessionLoader)

