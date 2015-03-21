QT += widgets
DEFINES += UNICODE _UNICODE
TEMPLATE        = lib
CONFIG         += plugin
win32 {
    LIBS += -ladvapi32
}

HEADERS         = \
    ../Ultracopier/StructEnumDefinition.h \
    ../Ultracopier/StructEnumDefinition_CopyEngine.h \
    ../Ultracopier/DebugEngineMacro.h \
    ../Ultracopier/Variable.h \
    ../Ultracopier/TransferThread.h \
    ../Ultracopier/ReadThread.h \
    ../Ultracopier/WriteThread.h \
    ../Ultracopier/MkPath.h \
    ../Ultracopier/AvancedQFile.h \
    ../Ultracopier/ListThread.h \
    ../../../interface/PluginInterface_CopyEngine.h \
    ../../../interface/OptionInterface.h \
    ../../../interface/FacilityInterface.h \
    ../Ultracopier/Filters.h \
    ../Ultracopier/FilterRules.h \
    ../Ultracopier/RenamingRules.h \
    ../Ultracopier/DriveManagement.h \
    ../Ultracopier/CopyEngine.h \
    ../Ultracopier/DebugDialog.h \
    ../Ultracopier/Factory.h \
    ../Ultracopier/FileErrorDialog.h \
    ../Ultracopier/FileExistsDialog.h \
    ../Ultracopier/FileIsSameDialog.h \
    ../Ultracopier/FolderExistsDialog.h \
    ../Ultracopier/ScanFileOrFolder.h \
    ../Ultracopier/DiskSpace.h
SOURCES         = \
    ../Ultracopier/TransferThread.cpp \
    ../Ultracopier/ReadThread.cpp \
    ../Ultracopier/WriteThread.cpp \
    ../Ultracopier/MkPath.cpp \
    ../Ultracopier/AvancedQFile.cpp \
    ../Ultracopier/ListThread.cpp \
    ../Ultracopier/Filters.cpp \
    ../Ultracopier/FilterRules.cpp \
    ../Ultracopier/RenamingRules.cpp \
    ../Ultracopier/ListThread_InodeAction.cpp \
    ../Ultracopier/DriveManagement.cpp \
    ../Ultracopier/CopyEngine-collision-and-error.cpp \
    ../Ultracopier/CopyEngine.cpp \
    ../Ultracopier/DebugDialog.cpp \
    ../Ultracopier/Factory.cpp \
    ../Ultracopier/FileErrorDialog.cpp \
    ../Ultracopier/FileExistsDialog.cpp \
    ../Ultracopier/FileIsSameDialog.cpp \
    ../Ultracopier/FolderExistsDialog.cpp \
    ../Ultracopier/ScanFileOrFolder.cpp \
    ../Ultracopier/DiskSpace.cpp
TARGET          = $$qtLibraryTarget(copyEngine)
include(../../../updateqm.pri)
TRANSLATIONS += ../Ultracopier/Languages/ar/translation.ts \
    ../Ultracopier/Languages/de/translation.ts \
    ../Ultracopier/Languages/el/translation.ts \
    ../Ultracopier/Languages/en/translation.ts \
    ../Ultracopier/Languages/es/translation.ts \
    ../Ultracopier/Languages/fr/translation.ts \
    ../Ultracopier/Languages/hi/translation.ts \
    ../Ultracopier/Languages/hu/translation.ts \
    ../Ultracopier/Languages/id/translation.ts \
    ../Ultracopier/Languages/it/translation.ts \
    ../Ultracopier/Languages/ja/translation.ts \
    ../Ultracopier/Languages/ko/translation.ts \
    ../Ultracopier/Languages/nl/translation.ts \
    ../Ultracopier/Languages/no/translation.ts \
    ../Ultracopier/Languages/pl/translation.ts \
    ../Ultracopier/Languages/pt/translation.ts \
    ../Ultracopier/Languages/ru/translation.ts \
    ../Ultracopier/Languages/th/translation.ts \
    ../Ultracopier/Languages/tr/translation.ts \
    ../Ultracopier/Languages/zh/translation.ts

include(../../../extratool.pri)
target.path     = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
translations.files = Languages
translations.path = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
infos.files      = informations.xml
infos.path       = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
INSTALLS       += target translations infos

FORMS += \
    ../Ultracopier/fileErrorDialog.ui \
    ../Ultracopier/fileExistsDialog.ui \
    ../Ultracopier/fileIsSameDialog.ui \
    ../Ultracopier/debugDialog.ui \
    ../Ultracopier/folderExistsDialog.ui \
    ../Ultracopier/Filters.ui \
    ../Ultracopier/FilterRules.ui \
    ../Ultracopier/RenamingRules.ui \
    ../Ultracopier/copyEngineOptions.ui \
    ../Ultracopier/DiskSpace.ui

OTHER_FILES += informations.xml

!CONFIG(static) {
RESOURCES += \
    ../Ultracopier/copyEngineResources.qrc
}
