CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

QT += widgets
TEMPLATE        = lib
CONFIG         += plugin
HEADERS         = \
    $$PWD/StructEnumDefinition.h \
    $$PWD/pluginLoader.h \
    $$PWD/DebugEngineMacro.h \
    $$PWD/Environment.h \
    $$PWD/Variable.h \
    $$PWD/PlatformMacro.h \
    $$PWD/../../../interface/PluginInterface_PluginLoader.h \
    $$PWD/OptionsWidget.h
SOURCES         = \
    $$PWD/pluginLoader.cpp \
    $$PWD/OptionsWidget.cpp
TARGET          = $$qtLibraryTarget(pluginLoader)
TRANSLATIONS += \
    $$PWD/Languages/ar/translation.ts \
    $$PWD/Languages/de/translation.ts \
    $$PWD/Languages/el/translation.ts \
    $$PWD/Languages/en/translation.ts \
    $$PWD/Languages/es/translation.ts \
    $$PWD/Languages/fr/translation.ts \
    $$PWD/Languages/hi/translation.ts \
    $$PWD/Languages/hu/translation.ts \
    $$PWD/Languages/id/translation.ts \
    $$PWD/Languages/it/translation.ts \
    $$PWD/Languages/ja/translation.ts \
    $$PWD/Languages/ko/translation.ts \
    $$PWD/Languages/nl/translation.ts \
    $$PWD/Languages/no/translation.ts \
    $$PWD/Languages/pl/translation.ts \
    $$PWD/Languages/pt/translation.ts \
    $$PWD/Languages/ru/translation.ts \
    $$PWD/Languages/th/translation.ts \
    $$PWD/Languages/tr/translation.ts \
    $$PWD/Languages/zh/translation.ts

win32:LIBS += -lole32 -lshell32

FORMS += \
    $$PWD/OptionsWidget.ui

CONFIG(static, static|shared) {
DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE
}
