TEMPLATE        = lib
CONFIG         += plugin
HEADERS         = interface.h \
		interface/PluginInterface_Themes.h \
		StructEnumDefinition.h \
    factory.h \
    DebugEngineMacro.h \
    Environment.h \
    Variable.h \
    ../../../interface/PluginInterface_Themes.h \
    TransferModel.h
SOURCES         = interface.cpp \
    factory.cpp \
    TransferModel.cpp
TARGET          = $$qtLibraryTarget(interface)
TRANSLATIONS   += Languages/fr/translation.ts \
    Languages/ar/translation.ts \
    Languages/ch/translation.ts \
    Languages/es/translation.ts \
    Languages/de/translation.ts \
    Languages/el/translation.ts \
    Languages/it/translation.ts \
    Languages/jp/translation.ts \
    Languages/id/translation.ts \
    Languages/pl/translation.ts \
    Languages/ru/translation.ts \
    Languages/tr/translation.ts \
    Languages/th/translation.ts \
    Languages/hi/translation.ts \
    Languages/nl/translation.ts \
    Languages/no/translation.ts \
    Languages/pt/translation.ts

RESOURCES	+= resources.qrc

FORMS += \
    interface.ui

win32 {
    RESOURCES += resources_windows.qrc
}
!win32 {
    RESOURCES += resources_unix.qrc
}
