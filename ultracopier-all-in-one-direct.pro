DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE

include(ultracopier-core.pro)

RESOURCES += plugins/static-plugins.qrc \
    plugins/CopyEngine/Ultracopier/copyEngineResources.qrc \
    plugins/Themes/Oxygen/interfaceResources_unix.qrc \
    plugins/Themes/Oxygen/interfaceResources_windows.qrc \
    plugins/Themes/Oxygen/interfaceResources.qrc

win32:RESOURCES += plugins/static-plugins-windows.qrc

HEADERS -= lib/qt-tar-xz/xz.h \
    lib/qt-tar-xz/QXzDecodeThread.h \
    lib/qt-tar-xz/QXzDecode.h \
    lib/qt-tar-xz/QTarDecode.h \
    AuthPlugin.h
SOURCES -= lib/qt-tar-xz/QXzDecodeThread.cpp \
    lib/qt-tar-xz/QXzDecode.cpp \
    lib/qt-tar-xz/QTarDecode.cpp \
    lib/qt-tar-xz/xz_crc32.c \
    lib/qt-tar-xz/xz_dec_stream.c \
    lib/qt-tar-xz/xz_dec_lzma2.c \
    lib/qt-tar-xz/xz_dec_bcj.c \
    AuthPlugin.cpp
INCLUDEPATH -= lib/qt-tar-xz/

RESOURCES -= resources/resources-windows-qt-plugin.qrc

DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE
DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT

FORMS += \
    plugins/CopyEngine/Ultracopier/copyEngineOptions.ui \
    plugins/CopyEngine/Ultracopier/debugDialog.ui \
    plugins/CopyEngine/Ultracopier/DiskSpace.ui \
    plugins/CopyEngine/Ultracopier/fileErrorDialog.ui \
    plugins/CopyEngine/Ultracopier/fileExistsDialog.ui \
    plugins/CopyEngine/Ultracopier/fileIsSameDialog.ui \
    plugins/CopyEngine/Ultracopier/FilterRules.ui \
    plugins/CopyEngine/Ultracopier/Filters.ui \
    plugins/CopyEngine/Ultracopier/folderExistsDialog.ui \
    plugins/CopyEngine/Ultracopier/RenamingRules.ui \
    plugins/Themes/Oxygen/themesOptions.ui \
    plugins/Themes/Oxygen/options.ui \
    plugins/Themes/Oxygen/interface.ui

HEADERS += \
    plugins/CopyEngine/Ultracopier/AvancedQFile.h \
    plugins/CopyEngine/Ultracopier/CompilerInfo.h \
    plugins/CopyEngine/Ultracopier/CopyEngine.h \
    plugins/CopyEngine/Ultracopier/DebugDialog.h \
    plugins/CopyEngine/Ultracopier/DebugEngineMacro.h \
    plugins/CopyEngine/Ultracopier/DiskSpace.h \
    plugins/CopyEngine/Ultracopier/DriveManagement.h \
    plugins/CopyEngine/Ultracopier/Environment.h \
    plugins/CopyEngine/Ultracopier/CopyEngineFactory.h \
    plugins/CopyEngine/Ultracopier/FileErrorDialog.h \
    plugins/CopyEngine/Ultracopier/FileExistsDialog.h \
    plugins/CopyEngine/Ultracopier/FileIsSameDialog.h \
    plugins/CopyEngine/Ultracopier/FilterRules.h \
    plugins/CopyEngine/Ultracopier/Filters.h \
    plugins/CopyEngine/Ultracopier/FolderExistsDialog.h \
    plugins/CopyEngine/Ultracopier/MkPath.h \
    plugins/CopyEngine/Ultracopier/ListThread.h \
    plugins/CopyEngine/Ultracopier/ReadThread.h \
    plugins/CopyEngine/Ultracopier/RenamingRules.h \
    plugins/CopyEngine/Ultracopier/ScanFileOrFolder.h \
    plugins/CopyEngine/Ultracopier/StructEnumDefinition_CopyEngine.h \
    plugins/CopyEngine/Ultracopier/StructEnumDefinition.h \
    plugins/CopyEngine/Ultracopier/TransferThread.h \
    plugins/CopyEngine/Ultracopier/Variable.h \
    plugins/CopyEngine/Ultracopier/WriteThread.h \
    plugins/Listener/catchcopy-v0002/Variable.h \
    plugins/Listener/catchcopy-v0002/StructEnumDefinition.h \
    plugins/Listener/catchcopy-v0002/listener.h \
    plugins/Listener/catchcopy-v0002/Environment.h \
    plugins/Listener/catchcopy-v0002/DebugEngineMacro.h \
    plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ClientCatchcopy.h \
    plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ExtraSocketCatchcopy.h \
    plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ServerCatchcopy.h \
    plugins/Listener/catchcopy-v0002/catchcopy-api-0002/VariablesCatchcopy.h \
    plugins/Themes/Oxygen/DebugEngineMacro.h \
    plugins/Themes/Oxygen/Environment.h \
    plugins/Themes/Oxygen/ThemesFactory.h \
    plugins/Themes/Oxygen/interface.h \
    plugins/Themes/Oxygen/Variable.h \
    plugins/Themes/Oxygen/TransferModel.h \
    plugins/Themes/Oxygen/StructEnumDefinition.h

SOURCES += \
    plugins/CopyEngine/Ultracopier/AvancedQFile.cpp \
    plugins/CopyEngine/Ultracopier/CopyEngine-collision-and-error.cpp \
    plugins/CopyEngine/Ultracopier/CopyEngine.cpp \
    plugins/CopyEngine/Ultracopier/DebugDialog.cpp \
    plugins/CopyEngine/Ultracopier/DiskSpace.cpp \
    plugins/CopyEngine/Ultracopier/DriveManagement.cpp \
    plugins/CopyEngine/Ultracopier/CopyEngineFactory.cpp \
    plugins/CopyEngine/Ultracopier/FileErrorDialog.cpp \
    plugins/CopyEngine/Ultracopier/FileExistsDialog.cpp \
    plugins/CopyEngine/Ultracopier/FileIsSameDialog.cpp \
    plugins/CopyEngine/Ultracopier/FilterRules.cpp \
    plugins/CopyEngine/Ultracopier/Filters.cpp \
    plugins/CopyEngine/Ultracopier/FolderExistsDialog.cpp \
    plugins/CopyEngine/Ultracopier/ListThread_InodeAction.cpp \
    plugins/CopyEngine/Ultracopier/MkPath.cpp \
    plugins/CopyEngine/Ultracopier/ReadThread.cpp \
    plugins/CopyEngine/Ultracopier/RenamingRules.cpp \
    plugins/CopyEngine/Ultracopier/ScanFileOrFolder.cpp \
    plugins/CopyEngine/Ultracopier/TransferThread.cpp \
    plugins/CopyEngine/Ultracopier/WriteThread.cpp \
    plugins/CopyEngine/Ultracopier/ListThread.cpp \
    plugins/Listener/catchcopy-v0002/listener.cpp \
    plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ClientCatchcopy.cpp \
    plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ExtraSocketCatchcopy.cpp \
    plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ServerCatchcopy.cpp \
    plugins/Themes/Oxygen/ThemesFactory.cpp \
    plugins/Themes/Oxygen/interface.cpp \
    plugins/Themes/Oxygen/TransferModel.cpp
