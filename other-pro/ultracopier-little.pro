wasm: DEFINES += NOAUDIO
android: DEFINES += NOAUDIO
#DEFINES += NOAUDIO
!contains(DEFINES, NOAUDIO) {
QT += multimedia
unix:LIBS += -lopus
macx:LIBS += -lopus
win32:LIBS += -lopus
SOURCES += \
    $$PWD/../libogg/bitwise.c \
    $$PWD/../libogg/framing.c \
    $$PWD/../opusfile/info.c \
    $$PWD/../opusfile/internal.c \
    $$PWD/../opusfile/opusfile.c \
    $$PWD/../opusfile/stream.c \

HEADERS  += \
    $$PWD/../libogg/ogg.h \
    $$PWD/../libogg/os_types.h \
    $$PWD/../opusfile/internal.h \
    $$PWD/../opusfile/opusfile.h \
}

include(ultracopier-little.pri)

SOURCES += \
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngine-collision-and-error.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngine.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/DebugDialog.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/DiskSpace.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/DriveManagement.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngineFactory.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/FileErrorDialog.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/FileExistsDialog.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/FileIsSameDialog.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/FilterRules.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/Filters.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/FolderExistsDialog.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThread_InodeAction.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/MkPath.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/RenamingRules.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ScanFileOrFolder.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/TransferThread.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThread.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/async/TransferThreadAsync.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadActions.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadStat.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadScan.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadOptions.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadNew.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadMedia.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadListChange.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/async/ReadThread.cpp \
    ../plugins/CopyEngine/Ultracopier-Spec/async/WriteThread.cpp

RESOURCES += \
    ../plugins/CopyEngine/Ultracopier-Spec/copyEngineResources.qrc

FORMS += \
    ../plugins/CopyEngine/Ultracopier-Spec/copyEngineOptions.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/debugDialog.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/DiskSpace.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/fileErrorDialog.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/fileExistsDialog.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/fileIsSameDialog.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/FilterRules.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/Filters.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/folderExistsDialog.ui \
    ../plugins/CopyEngine/Ultracopier-Spec/RenamingRules.ui

HEADERS += \
    ../plugins/CopyEngine/Ultracopier-Spec/CompilerInfo.h \
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngine.h \
    ../plugins/CopyEngine/Ultracopier-Spec/DebugDialog.h \
    ../plugins/CopyEngine/Ultracopier-Spec/DebugEngineMacro.h \
    ../plugins/CopyEngine/Ultracopier-Spec/DiskSpace.h \
    ../plugins/CopyEngine/Ultracopier-Spec/DriveManagement.h \
    ../plugins/CopyEngine/Ultracopier-Spec/Environment.h \
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngineFactory.h \
    ../plugins/CopyEngine/Ultracopier-Spec/FileErrorDialog.h \
    ../plugins/CopyEngine/Ultracopier-Spec/FileExistsDialog.h \
    ../plugins/CopyEngine/Ultracopier-Spec/FileIsSameDialog.h \
    ../plugins/CopyEngine/Ultracopier-Spec/FilterRules.h \
    ../plugins/CopyEngine/Ultracopier-Spec/Filters.h \
    ../plugins/CopyEngine/Ultracopier-Spec/FolderExistsDialog.h \
    ../plugins/CopyEngine/Ultracopier-Spec/MkPath.h \
    ../plugins/CopyEngine/Ultracopier-Spec/ListThread.h \
    ../plugins/CopyEngine/Ultracopier-Spec/RenamingRules.h \
    ../plugins/CopyEngine/Ultracopier-Spec/ScanFileOrFolder.h \
    ../plugins/CopyEngine/Ultracopier-Spec/StructEnumDefinition_CopyEngine.h \
    ../plugins/CopyEngine/Ultracopier-Spec/StructEnumDefinition.h \
    ../plugins/CopyEngine/Ultracopier-Spec/TransferThread.h \
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngineUltracopier-SpecVariable.h \
    ../plugins/CopyEngine/Ultracopier-Spec/async/TransferThreadAsync.h \
    ../plugins/CopyEngine/Ultracopier-Spec/async/ReadThread.h \
    ../plugins/CopyEngine/Ultracopier-Spec/async/WriteThread.h

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/../android-sources

DISTFILES += \
    ../android-sources/AndroidManifest.xml
