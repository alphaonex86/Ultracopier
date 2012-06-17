TEMPLATE        = lib
CONFIG         += plugin
HEADERS         = interface.h \
		StructEnumDefinition.h \
    factory.h \
    DebugEngineMacro.h \
    Environment.h \
    Variable.h \
    ../../../interface/PluginInterface_Themes.h \
    ../../../interface/FacilityInterface.h \
    ../../../interface/OptionInterface.h \
    TransferModel.h
SOURCES         = interface.cpp \
    factory.cpp \
    TransferModel.cpp
TARGET          = $$qtLibraryTarget(interface)
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

win32 {
    RESOURCES += resources_windows.qrc
}
!win32 {
    RESOURCES += resources_unix.qrc
}

RESOURCES	+= resources.qrc

FORMS += \
    interface.ui \
    options.ui
