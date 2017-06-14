CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

QT += widgets
TEMPLATE        = lib
CONFIG         += plugin
HEADERS         = ../Oxygen/ThemesFactory.h \
                ../Oxygen/StructEnumDefinition.h \
    ../Oxygen/DebugEngineMacro.h \
    ../Oxygen/Environment.h \
    ../Oxygen/Variable.h \
    ../../../interface/PluginInterface_Themes.h \
    ../../../interface/FacilityInterface.h \
    ../../../interface/OptionInterface.h \
    ../Oxygen/TransferModel.h \
    ../Oxygen/interface.h
SOURCES         = ../Oxygen/ThemesFactory.cpp \
    ../Oxygen/TransferModel.cpp \
    ../Oxygen/interface.cpp
TARGET          = $$qtLibraryTarget(interface)
TRANSLATIONS += ../Oxygen/Languages/ar/translation.ts \
    ../Oxygen/Languages/de/translation.ts \
    ../Oxygen/Languages/el/translation.ts \
    ../Oxygen/Languages/en/translation.ts \
    ../Oxygen/Languages/es/translation.ts \
    ../Oxygen/Languages/fr/translation.ts \
    ../Oxygen/Languages/hi/translation.ts \
    ../Oxygen/Languages/hu/translation.ts \
    ../Oxygen/Languages/id/translation.ts \
    ../Oxygen/Languages/it/translation.ts \
    ../Oxygen/Languages/ja/translation.ts \
    ../Oxygen/Languages/ko/translation.ts \
    ../Oxygen/Languages/nl/translation.ts \
    ../Oxygen/Languages/no/translation.ts \
    ../Oxygen/Languages/pl/translation.ts \
    ../Oxygen/Languages/pt/translation.ts \
    ../Oxygen/Languages/ru/translation.ts \
    ../Oxygen/Languages/th/translation.ts \
    ../Oxygen/Languages/tr/translation.ts \
    ../Oxygen/Languages/zh/translation.ts

!CONFIG(static) {
RESOURCES	+= \
    ../Oxygen/interfaceResources.qrc \
    ../Oxygen/interfaceResources_unix.qrc \
    ../Oxygen/interfaceResources_windows.qrc
}

FORMS += \
    ../Oxygen/interface.ui \
    ../Oxygen/themesOptions.ui
