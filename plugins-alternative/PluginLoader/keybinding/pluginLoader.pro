CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

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
    OptionsWidget.h \
    KeyBind.h
SOURCES         = \
    pluginLoader.cpp \
    OptionsWidget.cpp \
    KeyBind.cpp
TARGET          = $$qtLibraryTarget(pluginLoader)
TRANSLATIONS += Languages/ar/translation.ts \
    Languages/de/translation.ts \
    Languages/el/translation.ts \
    Languages/en/translation.ts \
    Languages/es/translation.ts \
    Languages/fr/translation.ts \
    Languages/hi/translation.ts \
    Languages/hu/translation.ts \
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

FORMS += \
    OptionsWidget.ui

CONFIG(static, static|shared) {
DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE
}

