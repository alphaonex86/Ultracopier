TEMPLATE        = lib
CONFIG         += plugin
HEADERS         = interface.h \
		interface/PluginInterface_Themes.h \
		StructEnumDefinition.h \
    factory.h \
    DebugEngineMacro.h \
    Environment.h \
    Variable.h
SOURCES         = interface.cpp \
    factory.cpp
TARGET          = $$qtLibraryTarget(interface)
TRANSLATIONS   += Languages/fr/translation.ts \
    resources/Languages/ar/translation.ts \
    resources/Languages/ch/translation.ts \
    resources/Languages/es/translation.ts \
    resources/Languages/de/translation.ts \
    resources/Languages/el/translation.ts \
    resources/Languages/it/translation.ts \
    resources/Languages/jp/translation.ts \
    resources/Languages/id/translation.ts \
    resources/Languages/pl/translation.ts \
    resources/Languages/ru/translation.ts \
    resources/Languages/tr/translation.ts \
    resources/Languages/th/translation.ts \
    resources/Languages/hi/translation.ts \
    resources/Languages/nl/translation.ts \
    resources/Languages/no/translation.ts \
    resources/Languages/pt/translation.ts

RESOURCES	+= resources.qrc

FORMS += \
    interface.ui
