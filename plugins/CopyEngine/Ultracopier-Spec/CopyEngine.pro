CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

QT += widgets xml
DEFINES += _FILE_OFFSET_BITS=64 UNICODE _UNICODE WIDESTRING
TEMPLATE        = lib
CONFIG         += plugin

HEADERS         += \
    $$PWD/StructEnumDefinition.h \
    $$PWD/StructEnumDefinition_CopyEngine.h \
    $$PWD/DebugEngineMacro.h \
    $$PWD/CopyEngineUltracopier-SpecVariable.h \
    $$PWD/TransferThread.h \
    $$PWD/MkPath.h \
    $$PWD/ListThread.h \
    $$PWD/../../../interface/PluginInterface_CopyEngine.h \
    $$PWD/../../../interface/OptionInterface.h \
    $$PWD/../../../interface/FacilityInterface.h \
    $$PWD/../../../cpp11addition.h \
    $$PWD/Filters.h \
    $$PWD/FilterRules.h \
    $$PWD/RenamingRules.h \
    $$PWD/DriveManagement.h \
    $$PWD/CopyEngine.h \
    $$PWD/DebugDialog.h \
    $$PWD/CopyEngineFactory.h \
    $$PWD/FileErrorDialog.h \
    $$PWD/FileExistsDialog.h \
    $$PWD/FileIsSameDialog.h \
    $$PWD/FolderExistsDialog.h \
    $$PWD/ScanFileOrFolder.h \
    $$PWD/DiskSpace.h
SOURCES         += \
    $$PWD/TransferThread.cpp \
    $$PWD/MkPath.cpp \
    $$PWD/ListThread.cpp \
    $$PWD/../../../cpp11addition.cpp \
    $$PWD/../../../cpp11additionstringtointcpp.cpp \
    $$PWD/Filters.cpp \
    $$PWD/FilterRules.cpp \
    $$PWD/RenamingRules.cpp \
    $$PWD/ListThread_InodeAction.cpp \
    $$PWD/DriveManagement.cpp \
    $$PWD/CopyEngine-collision-and-error.cpp \
    $$PWD/CopyEngine.cpp \
    $$PWD/DebugDialog.cpp \
    $$PWD/CopyEngineFactory.cpp \
    $$PWD/FileErrorDialog.cpp \
    $$PWD/FileExistsDialog.cpp \
    $$PWD/FileIsSameDialog.cpp \
    $$PWD/FolderExistsDialog.cpp \
    $$PWD/ScanFileOrFolder.cpp \
    $$PWD/DiskSpace.cpp
TARGET          = $$qtLibraryTarget(copyEngine)
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

FORMS += \
    $$PWD/fileErrorDialog.ui \
    $$PWD/fileExistsDialog.ui \
    $$PWD/fileIsSameDialog.ui \
    $$PWD/debugDialog.ui \
    $$PWD/folderExistsDialog.ui \
    $$PWD/Filters.ui \
    $$PWD/FilterRules.ui \
    $$PWD/RenamingRules.ui \
    $$PWD/copyEngineOptions.ui \
    $$PWD/DiskSpace.ui

OTHER_FILES += \
    $$PWD/informations.xml

!CONFIG(static) {
RESOURCES += \
    $$PWD/copyEngineResources.qrc
}

win32 {
    LIBS += -ladvapi32
}
HEADERS         += $$PWD/async/TransferThreadAsync.h
SOURCES         += $$PWD/async/TransferThreadAsync.cpp
