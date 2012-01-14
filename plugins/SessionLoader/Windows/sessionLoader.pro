TEMPLATE        = lib
CONFIG         += plugin
HEADERS         = sessionLoader.h \
    SessionLoader_PluginInterface.h \
    StructEnumDefinition.h \
    interface/PluginInterface_SessionLoader.h \
    Variable.h \
    Environment.h \
    DebugEngineMacro.h
SOURCES         = sessionLoader.cpp
TARGET          = $$qtLibraryTarget(sessionLoader)
TRANSLATIONS   += Languages/fr/translation.ts
