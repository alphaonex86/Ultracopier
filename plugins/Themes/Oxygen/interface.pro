CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

QT += widgets
TEMPLATE        = lib
CONFIG         += plugin
HEADERS         = \
    $$PWD/ThemesFactory.h \
    $$PWD/StructEnumDefinition.h \
    $$PWD/DebugEngineMacro.h \
    $$PWD/Environment.h \
    $$PWD/Variable.h \
    $$PWD/../../../interface/PluginInterface_Themes.h \
    $$PWD/../../../interface/FacilityInterface.h \
    $$PWD/../../../interface/OptionInterface.h \
    $$PWD/../../../cpp11addition.h \
    $$PWD/TransferModel.h \
    $$PWD/interface.h
SOURCES         = \
    $$PWD/ThemesFactory.cpp \
    $$PWD/TransferModel.cpp \
    $$PWD/interface.cpp \
    $$PWD/../../../cpp11addition.cpp \
    $$PWD/../../../cpp11additionstringtointcpp.cpp
TARGET          = $$qtLibraryTarget(interface)
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

!defined(SUPERCOPIER) {
    !CONFIG(static) {
    RESOURCES	+= \
        $$PWD/../Oxygen/interfaceResources.qrc \
        $$PWD/../Oxygen/interfaceResources_unix.qrc \
        $$PWD/../Oxygen/interfaceResources_windows.qrc
    }
}

FORMS += \
    $$PWD/interface.ui \
    $$PWD/themesOptions.ui
