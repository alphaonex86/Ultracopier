DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE

include(other-pro/ultracopier-core.pro)

RESOURCES += $$PWD/plugins/static-plugins.qrc \
    $$PWD/plugins/CopyEngine/Ultracopier/copyEngineResources.qrc \
    $$PWD/plugins/Themes/Oxygen/interfaceResources_unix.qrc \
    $$PWD/plugins/Themes/Oxygen/interfaceResources_windows.qrc \
    $$PWD/plugins/Themes/Oxygen/interfaceResources.qrc

win32:RESOURCES += $$PWD/plugins/static-plugins-windows.qrc

HEADERS -= $$PWD/lib/qt-tar-xz/xz.h \
    $$PWD/lib/qt-tar-xz/QXzDecodeThread.h \
    $$PWD/lib/qt-tar-xz/QXzDecode.h \
    $$PWD/lib/qt-tar-xz/QTarDecode.h \
    $$PWD/AuthPlugin.h
SOURCES -= $$PWD/lib/qt-tar-xz/QXzDecodeThread.cpp \
    $$PWD/lib/qt-tar-xz/QXzDecode.cpp \
    $$PWD/lib/qt-tar-xz/QTarDecode.cpp \
    $$PWD/lib/qt-tar-xz/xz_crc32.c \
    $$PWD/lib/qt-tar-xz/xz_dec_stream.c \
    $$PWD/lib/qt-tar-xz/xz_dec_lzma2.c \
    $$PWD/lib/qt-tar-xz/xz_dec_bcj.c \
    $$PWD/AuthPlugin.cpp
INCLUDEPATH -= $$PWD/lib/qt-tar-xz/

RESOURCES -= $$PWD/resources/resources-windows-qt-plugin.qrc

DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE
DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT

FORMS += \
    $$PWD/plugins/CopyEngine/Ultracopier/copyEngineOptions.ui \
    $$PWD/plugins/CopyEngine/Ultracopier/debugDialog.ui \
    $$PWD/plugins/CopyEngine/Ultracopier/DiskSpace.ui \
    $$PWD/plugins/CopyEngine/Ultracopier/fileErrorDialog.ui \
    $$PWD/plugins/CopyEngine/Ultracopier/fileExistsDialog.ui \
    $$PWD/plugins/CopyEngine/Ultracopier/fileIsSameDialog.ui \
    $$PWD/plugins/CopyEngine/Ultracopier/FilterRules.ui \
    $$PWD/plugins/CopyEngine/Ultracopier/Filters.ui \
    $$PWD/plugins/CopyEngine/Ultracopier/folderExistsDialog.ui \
    $$PWD/plugins/CopyEngine/Ultracopier/RenamingRules.ui \
    $$PWD/plugins/Themes/Oxygen/themesOptions.ui \
    $$PWD/plugins/Themes/Oxygen/options.ui \
    $$PWD/plugins/Themes/Oxygen/interface.ui

HEADERS += \
    $$PWD/plugins/CopyEngine/Ultracopier/AvancedQFile.h \
    $$PWD/plugins/CopyEngine/Ultracopier/CompilerInfo.h \
    $$PWD/plugins/CopyEngine/Ultracopier/CopyEngine.h \
    $$PWD/plugins/CopyEngine/Ultracopier/DebugDialog.h \
    $$PWD/plugins/CopyEngine/Ultracopier/DebugEngineMacro.h \
    $$PWD/plugins/CopyEngine/Ultracopier/DiskSpace.h \
    $$PWD/plugins/CopyEngine/Ultracopier/DriveManagement.h \
    $$PWD/plugins/CopyEngine/Ultracopier/Environment.h \
    $$PWD/plugins/CopyEngine/Ultracopier/CopyEngineFactory.h \
    $$PWD/plugins/CopyEngine/Ultracopier/FileErrorDialog.h \
    $$PWD/plugins/CopyEngine/Ultracopier/FileExistsDialog.h \
    $$PWD/plugins/CopyEngine/Ultracopier/FileIsSameDialog.h \
    $$PWD/plugins/CopyEngine/Ultracopier/FilterRules.h \
    $$PWD/plugins/CopyEngine/Ultracopier/Filters.h \
    $$PWD/plugins/CopyEngine/Ultracopier/FolderExistsDialog.h \
    $$PWD/plugins/CopyEngine/Ultracopier/MkPath.h \
    $$PWD/plugins/CopyEngine/Ultracopier/ListThread.h \
    $$PWD/plugins/CopyEngine/Ultracopier/ReadThread.h \
    $$PWD/plugins/CopyEngine/Ultracopier/RenamingRules.h \
    $$PWD/plugins/CopyEngine/Ultracopier/ScanFileOrFolder.h \
    $$PWD/plugins/CopyEngine/Ultracopier/StructEnumDefinition_CopyEngine.h \
    $$PWD/plugins/CopyEngine/Ultracopier/StructEnumDefinition.h \
    $$PWD/plugins/CopyEngine/Ultracopier/TransferThread.h \
    $$PWD/plugins/CopyEngine/Ultracopier/Variable.h \
    $$PWD/plugins/CopyEngine/Ultracopier/WriteThread.h \
    $$PWD/plugins/Listener/catchcopy-v0002/Variable.h \
    $$PWD/plugins/Listener/catchcopy-v0002/StructEnumDefinition.h \
    $$PWD/plugins/Listener/catchcopy-v0002/listener.h \
    $$PWD/plugins/Listener/catchcopy-v0002/Environment.h \
    $$PWD/plugins/Listener/catchcopy-v0002/DebugEngineMacro.h \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ClientCatchcopy.h \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ExtraSocketCatchcopy.h \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ServerCatchcopy.h \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/VariablesCatchcopy.h \
    $$PWD/plugins/Themes/Oxygen/DebugEngineMacro.h \
    $$PWD/plugins/Themes/Oxygen/Environment.h \
    $$PWD/plugins/Themes/Oxygen/ThemesFactory.h \
    $$PWD/plugins/Themes/Oxygen/interface.h \
    $$PWD/plugins/Themes/Oxygen/Variable.h \
    $$PWD/plugins/Themes/Oxygen/TransferModel.h \
    $$PWD/plugins/Themes/Oxygen/StructEnumDefinition.h

SOURCES += \
    $$PWD/plugins/CopyEngine/Ultracopier/AvancedQFile.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/CopyEngine-collision-and-error.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/CopyEngine.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/DebugDialog.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/DiskSpace.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/DriveManagement.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/CopyEngineFactory.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/FileErrorDialog.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/FileExistsDialog.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/FileIsSameDialog.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/FilterRules.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/Filters.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/FolderExistsDialog.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/ListThread_InodeAction.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/MkPath.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/ReadThread.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/RenamingRules.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/ScanFileOrFolder.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/TransferThread.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/WriteThread.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier/ListThread.cpp \
    $$PWD/plugins/Listener/catchcopy-v0002/listener.cpp \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ClientCatchcopy.cpp \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ExtraSocketCatchcopy.cpp \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ServerCatchcopy.cpp \
    $$PWD/plugins/Themes/Oxygen/ThemesFactory.cpp \
    $$PWD/plugins/Themes/Oxygen/interface.cpp \
    $$PWD/plugins/Themes/Oxygen/TransferModel.cpp
