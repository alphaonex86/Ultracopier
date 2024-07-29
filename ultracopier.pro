DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE
#DEFINES += ULTRACOPIER_DEBUG ULTRACOPIER_PLUGIN_DEBUG ULTRACOPIER_PLUGIN_DEBUG_WINDOW

include(other-pro/ultracopier-core.pro)

RESOURCES += $$PWD/plugins/static-plugins.qrc \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/copyEngineResources.qrc \
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
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/copyEngineOptions.ui \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/debugDialog.ui \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/DiskSpace.ui \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/fileErrorDialog.ui \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/fileExistsDialog.ui \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/fileIsSameDialog.ui \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/FilterRules.ui \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/Filters.ui \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/folderExistsDialog.ui \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/RenamingRules.ui \
    $$PWD/plugins/Themes/Oxygen/interface.ui \
    $$PWD/plugins/Themes/Oxygen/options.ui \
    $$PWD/plugins/Themes/Oxygen/themesOptions.ui

HEADERS += \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/CompilerInfo.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/CopyEngine.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/DebugDialog.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/DebugEngineMacro.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/DiskSpace.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/DriveManagement.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/Environment.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/CopyEngineFactory.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/FileErrorDialog.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/FileExistsDialog.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/FileIsSameDialog.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/FilterRules.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/Filters.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/FolderExistsDialog.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/MkPath.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/ListThread.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/RenamingRules.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/ScanFileOrFolder.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/StructEnumDefinition_CopyEngine.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/StructEnumDefinition.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/TransferThread.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/CopyEngineUltracopier-SpecVariable.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/async/ReadThread.h \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/async/WriteThread.h \
    $$PWD/plugins/Listener/catchcopy-v0002/Listenercatchcopy-v0002Variable.h \
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
    $$PWD/plugins/Themes/Oxygen/interface.h \
    $$PWD/plugins/Themes/Oxygen/OxygenVariable.h \
    $$PWD/plugins/Themes/Oxygen/StructEnumDefinition.h \
    $$PWD/plugins/Themes/Oxygen/ThemesFactory.h \
    $$PWD/plugins/Themes/Oxygen/TransferModel.h

SOURCES += \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/CopyEngine-collision-and-error.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/CopyEngine.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/DebugDialog.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/DiskSpace.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/DriveManagement.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/CopyEngineFactory.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/FileErrorDialog.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/FileExistsDialog.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/FileIsSameDialog.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/FilterRules.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/Filters.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/FolderExistsDialog.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/ListThread_InodeAction.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/MkPath.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/RenamingRules.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/ScanFileOrFolder.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/TransferThread.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/ListThread.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/ListThreadListChange.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/ListThreadActions.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/ListThreadMedia.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/ListThreadNew.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/ListThreadOptions.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/ListThreadScan.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/ListThreadStat.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/async/ReadThread.cpp \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/async/WriteThread.cpp \
    $$PWD/plugins/Listener/catchcopy-v0002/listener.cpp \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ClientCatchcopy.cpp \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ExtraSocketCatchcopy.cpp \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ServerCatchcopy.cpp \
    $$PWD/plugins/Themes/Oxygen/interface.cpp \
    $$PWD/plugins/Themes/Oxygen/ThemesFactory.cpp \
    $$PWD/plugins/Themes/Oxygen/TransferModel.cpp

win32 {
    RESOURCES -= $$PWD/resources/resources-windows-qt-plugin.qrc

    HEADERS         += \
        $$PWD/plugins/PluginLoader/catchcopy-v0002/StructEnumDefinition.h \
        $$PWD/plugins/PluginLoader/catchcopy-v0002/pluginLoader.h \
        $$PWD/plugins/PluginLoader/catchcopy-v0002/DebugEngineMacro.h \
        $$PWD/plugins/PluginLoader/catchcopy-v0002/Environment.h \
        $$PWD/plugins/PluginLoader/catchcopy-v0002/PluginLoadercatchcopy-v0002Variable.h \
        $$PWD/plugins/PluginLoader/catchcopy-v0002/PlatformMacro.h \
        $$PWD/plugins/PluginLoader/catchcopy-v0002/OptionsWidget.h
    SOURCES         += \
        $$PWD/plugins/PluginLoader/catchcopy-v0002/pluginLoader.cpp \
        $$PWD/plugins/PluginLoader/catchcopy-v0002/OptionsWidget.cpp
    FORMS += $$PWD/plugins/PluginLoader/catchcopy-v0002/OptionsWidget.ui
    LIBS += -lole32 -lshell32
}

#temp
HEADERS += $$PWD/plugins/CopyEngine/Ultracopier-Spec/async/TransferThreadAsync.h
SOURCES += $$PWD/plugins/CopyEngine/Ultracopier-Spec/async/TransferThreadAsync.cpp
