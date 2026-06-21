CONFIG += c++17
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"

QT += widgets xml
DEFINES += _FILE_OFFSET_BITS=64 UNICODE _UNICODE _LARGE_FILE_SOURCE=1
# io_uring needs liburing at build time; fall back to async when it is not installed.
linux:packagesExist(liburing): DEFINES += ULTRACOPIER_PLUGIN_IO_URING
# IOCP needs Windows Vista+ APIs (GetQueuedCompletionStatusEx/CancelIoEx), absent on XP.
# Qt6 only runs on Windows 10+, so gate IOCP on Qt6; Qt5/XP builds use the async backend.
win32:greaterThan(QT_MAJOR_VERSION, 5): DEFINES += ULTRACOPIER_PLUGIN_WINIOCP
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
    $$PWD/DiskSpace.cpp \
    $$PWD/ListThreadActions.cpp \
    $$PWD/ListThreadListChange.cpp \
    $$PWD/ListThreadMedia.cpp \
    $$PWD/ListThreadNew.cpp \
    $$PWD/ListThreadOptions.cpp \
    $$PWD/ListThreadScan.cpp \
    $$PWD/ListThreadStat.cpp
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
    DEFINES += WIDESTRING
}

# xxhash: prefer system header if available, otherwise fall back to bundled vendor copy.
# Under Windows builds (native MinGW, wine-hosted MinGW, MXE cross) the host's
# /usr/include/xxhash.h is a Linux header that the Windows toolchain cannot use,
# so always pick the vendored copy. On other platforms, probe the usual prefixes.
XXHASH_USE_VENDOR = 1
!win32 {
    exists(/usr/include/xxhash.h):       XXHASH_USE_VENDOR = 0
    exists(/usr/local/include/xxhash.h): XXHASH_USE_VENDOR = 0
    exists(/opt/local/include/xxhash.h): XXHASH_USE_VENDOR = 0
}
equals(XXHASH_USE_VENDOR, 1) {
    INCLUDEPATH += $$PWD/../../../lib/xxhash
    HEADERS += $$PWD/../../../lib/xxhash/xxhash.h
    SOURCES += $$PWD/../../../lib/xxhash/xxhash.c
} else {
    LIBS += -lxxhash
}

contains(DEFINES, ULTRACOPIER_PLUGIN_WINIOCP) {
    # Windows IOCP backend (analog of io_uring). Single TransferThread per inode,
    # no separate Read/Write threads. Shares all transfer logic with the io_uring
    # backend via TransferThreadPipelined; only the I/O layer differs. The backend
    # .cpp/.h are wrapped in #ifdef ULTRACOPIER_PLUGIN_WINIOCP so they compile to nothing elsewhere.
    HEADERS += $$PWD/pipeline/TransferThreadPipelined.h \
               $$PWD/win-iocp/TransferThreadWin.h
    SOURCES += $$PWD/pipeline/TransferThreadPipelined.cpp \
               $$PWD/win-iocp/TransferThreadWin.cpp
} else:contains(DEFINES, ULTRACOPIER_PLUGIN_IO_URING) {
    HEADERS += $$PWD/pipeline/TransferThreadPipelined.h \
               $$PWD/uring/TransferThreadUring.h
    SOURCES += $$PWD/pipeline/TransferThreadPipelined.cpp \
               $$PWD/uring/TransferThreadUring.cpp
    LIBS += -luring
} else {
    HEADERS += $$PWD/async/ReadThread.h \
               $$PWD/async/WriteThread.h \
               $$PWD/async/TransferThreadAsync.h
    SOURCES += $$PWD/async/ReadThread.cpp \
               $$PWD/async/WriteThread.cpp \
               $$PWD/async/TransferThreadAsync.cpp
}

contains(DEFINES, ULTRACOPIER_PLUGIN_KIO) {
    CONFIG += link_pkgconfig
    PKGCONFIG += KF6KIOCore
    SOURCES += $$PWD/ListThreadKio.cpp
}
