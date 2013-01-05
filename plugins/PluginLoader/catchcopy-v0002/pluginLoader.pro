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
include(../../../updateqm.pri)
TRANSLATIONS += Languages/ar/translation.ts \
    Languages/de/translation.ts \
    Languages/el/translation.ts \
    Languages/es/translation.ts \
    Languages/fr/translation.ts \
    Languages/hi/translation.ts \
    Languages/id/translation.ts \
    Languages/it/translation.ts \
    Languages/ja/translation.ts \
    Languages/ko/translation.ts \
    Languages/nl/translation.ts \
    Languages/no/translation.ts \
    Languages/pl/translation.ts \
    Languages/pt/translation.ts \
    Languages/ru/translation.ts \
    Languages/th/translation.ts \
    Languages/tr/translation.ts \
    Languages/zh/translation.ts

include(../../../extratool.pri)
target.path     = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
translations.files = Languages
translations.path = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
infos.files      = informations.xml
infos.path       = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
INSTALLS       += target translations infos

LIBS += -lole32

FORMS += \
    OptionsWidget.ui

