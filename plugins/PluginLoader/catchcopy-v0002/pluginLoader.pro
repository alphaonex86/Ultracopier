QT += widgets
TEMPLATE        = lib
CONFIG         += plugin
HEADERS         = \
    StructEnumDefinition.h \
    pluginLoader.h \
    DebugEngineMacro.h \
    Environment.h \
    Variable.h \
    PlatformMacro.h \
    ../../../interface/PluginInterface_PluginLoader.h \
    OptionsWidget.h
SOURCES         = \
    pluginLoader.cpp \
    OptionsWidget.cpp
TARGET          = $$qtLibraryTarget(pluginLoader)
TRANSLATIONS += Languages/fr/translation.ts \
    Languages/ar/translation.ts \
    Languages/zh/translation.ts \
    Languages/es/translation.ts \
    Languages/de/translation.ts \
    Languages/el/translation.ts \
    Languages/it/translation.ts \
    Languages/ja/translation.ts \
    Languages/id/translation.ts \
    Languages/pl/translation.ts \
    Languages/ru/translation.ts \
    Languages/tr/translation.ts \
    Languages/th/translation.ts \
    Languages/hi/translation.ts \
    Languages/nl/translation.ts \
    Languages/no/translation.ts \
    Languages/pt/translation.ts \
    Languages/ko/translation.ts

LIBS += -lole32

FORMS += \
    OptionsWidget.ui

