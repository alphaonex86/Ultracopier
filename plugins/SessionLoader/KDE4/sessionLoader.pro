TEMPLATE        = lib
CONFIG         += plugin
HEADERS         = sessionLoader.h \
    StructEnumDefinition.h \
    Variable.h \
    Environment.h \
    DebugEngineMacro.h
SOURCES         = sessionLoader.cpp
TARGET          = $$qtLibraryTarget(sessionLoader)
TRANSLATIONS   += Languages/fr/translation.ts
