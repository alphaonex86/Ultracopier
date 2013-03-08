QT += widgets systeminfo
TEMPLATE        = lib
CONFIG         += plugin static
HEADERS         = \
                StructEnumDefinition.h \
    StructEnumDefinition_CopyEngine.h \
    DebugEngineMacro.h \
    Variable.h \
    TransferThread.h \
    ReadThread.h \
    WriteThread.h \
    MkPath.h \
    AvancedQFile.h \
    ListThread.h \
    ../../../interface/PluginInterface_CopyEngine.h \
    ../../../interface/OptionInterface.h \
    ../../../interface/FacilityInterface.h \
    Filters.h \
    FilterRules.h \
    RenamingRules.h \
    DriveManagement.h \
    CopyEngine.h \
    DebugDialog.h \
    Factory.h \
    FileErrorDialog.h \
    FileExistsDialog.h \
    FileIsSameDialog.h \
    FolderExistsDialog.h \
    ScanFileOrFolder.h
SOURCES         = \
    TransferThread.cpp \
    ReadThread.cpp \
    WriteThread.cpp \
    MkPath.cpp \
    AvancedQFile.cpp \
    ListThread.cpp \
    Filters.cpp \
    FilterRules.cpp \
    RenamingRules.cpp \
    ListThread_InodeAction.cpp \
    DriveManagement.cpp \
    CopyEngine-collision-and-error.cpp \
    CopyEngine.cpp \
    DebugDialog.cpp \
    Factory.cpp \
    FileErrorDialog.cpp \
    FileExistsDialog.cpp \
    FileIsSameDialog.cpp \
    FolderExistsDialog.cpp \
    ScanFileOrFolder.cpp
TARGET          = $$qtLibraryTarget(copyEngine)
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

include(../../../extratool.pri)
target.path     = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
translations.files = Languages
translations.path = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
infos.files      = informations.xml
infos.path       = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
INSTALLS       += target translations infos

FORMS += \
    fileErrorDialog.ui \
    fileExistsDialog.ui \
    fileIsSameDialog.ui \
    debugDialog.ui \
    folderExistsDialog.ui \
    Filters.ui \
    FilterRules.ui \
    RenamingRules.ui \
    copyEngineOptions.ui

OTHER_FILES += informations.xml

RESOURCES += \
    copyEngineResources.qrc
