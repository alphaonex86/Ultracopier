QT += widgets
TEMPLATE        = lib
CONFIG         += plugin
HEADERS         = ThemesFactory.h \
                StructEnumDefinition.h \
    DebugEngineMacro.h \
    Environment.h \
    Variable.h \
    ../../../interface/PluginInterface_Themes.h \
    ../../../interface/FacilityInterface.h \
    ../../../interface/OptionInterface.h \
    TransferModel.h
SOURCES         = ThemesFactory.cpp \
    TransferModel.cpp
TARGET          = $$qtLibraryTarget(interface)
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

win32 {
    RESOURCES +=
}
!win32 {
    RESOURCES +=
}

!CONFIG(static) {
RESOURCES	+= \
    interfaceResources.qrc \
    interfaceResources_unix.qrc \
    interfaceResources_windows.qrc
}

FORMS += \
    interface.ui \
    themesOptions.ui
