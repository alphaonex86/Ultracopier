QT += widgets
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
include(../../../extratool.pri)
target.path     = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
include(../../../updateqm.pri)
TRANSLATIONS += Languages/ar/translation.ts \
    Languages/de/translation.ts \
    Languages/el/translation.ts \
    Languages/en/translation.ts \
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

translations.files = Languages
translations.path = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
infos.files      = informations.xml
infos.path       = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
INSTALLS       += target translations infos

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
