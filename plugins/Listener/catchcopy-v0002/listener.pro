TEMPLATE        = lib
CONFIG		+= plugin
QT		+= network
HEADERS         = listener.h \
		PluginInterface_Listener.h \
    catchcopy-api-0002/VariablesCatchcopy.h \
    catchcopy-api-0002/ServerCatchcopy.h \
    catchcopy-api-0002/ExtraSocketCatchcopy.h \
    Environment.h \
    Variable.h \
    DebugEngineMacro.h \
    interface/PluginInterface_Listener.h \
    StructEnumDefinition.h
SOURCES         = listener.cpp \
    catchcopy-api-0002/ServerCatchcopy.cpp \
    catchcopy-api-0002/ExtraSocketCatchcopy.cpp
TARGET          = $$qtLibraryTarget(listener)
TRANSLATIONS   += Languages/fr/translation.ts
