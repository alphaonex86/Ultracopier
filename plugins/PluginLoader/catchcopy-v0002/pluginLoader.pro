TEMPLATE        = lib
CONFIG         += plugin
HEADERS         = \
    StructEnumDefinition.h \
    pluginLoader.h \
    DebugEngineMacro.h \
    Environment.h \
    Variable.h \
    ../../../interface/PluginInterface_PluginLoader.h \
    PlatformMacro.h
SOURCES         = \
    pluginLoader.cpp
TARGET          = $$qtLibraryTarget(pluginLoader)
TRANSLATIONS   += Languages/fr/translation.ts
