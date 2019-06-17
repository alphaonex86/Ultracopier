DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE

include(other-pro/ultracopier-core.pro)

RESOURCES += $$PWD/plugins/static-plugins.qrc \
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/copyEngineResources.qrc \
    $$PWD/plugins/Themes/Oxygen2/interfaceResources_unix.qrc \
    $$PWD/plugins/Themes/Oxygen2/interfaceResources_windows.qrc \
    $$PWD/plugins/Themes/Oxygen2/interfaceResources.qrc

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
    $$PWD/plugins/Themes/Oxygen2/themesOptions.ui \
    $$PWD/plugins/Themes/Oxygen2/options.ui \
    $$PWD/plugins/Themes/Oxygen2/interface.ui

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
    $$PWD/plugins/CopyEngine/Ultracopier-Spec/Variable.h \
    $$PWD/plugins/Listener/catchcopy-v0002/Variable.h \
    $$PWD/plugins/Listener/catchcopy-v0002/StructEnumDefinition.h \
    $$PWD/plugins/Listener/catchcopy-v0002/listener.h \
    $$PWD/plugins/Listener/catchcopy-v0002/Environment.h \
    $$PWD/plugins/Listener/catchcopy-v0002/DebugEngineMacro.h \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ClientCatchcopy.h \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ExtraSocketCatchcopy.h \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ServerCatchcopy.h \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/VariablesCatchcopy.h \
    $$PWD/plugins/Themes/Oxygen2/DebugEngineMacro.h \
    $$PWD/plugins/Themes/Oxygen2/Environment.h \
    $$PWD/plugins/Themes/Oxygen2/ThemesFactory.h \
    $$PWD/plugins/Themes/Oxygen2/interface.h \
    $$PWD/plugins/Themes/Oxygen2/Variable.h \
    $$PWD/plugins/Themes/Oxygen2/TransferModel.h \
    $$PWD/plugins/Themes/Oxygen2/StructEnumDefinition.h \
    $$PWD/plugins/Themes/Oxygen2/chartarea.h \
    $$PWD/plugins/Themes/Oxygen2/fileTree.h \
    $$PWD/plugins/Themes/Oxygen2/ProgressBarDark.h \
    $$PWD/plugins/Themes/Oxygen2/radialMap/map.h \
    $$PWD/plugins/Themes/Oxygen2/radialMap/widget.h \
    $$PWD/plugins/Themes/Oxygen2/radialMap/radialMap.h \
    $$PWD/plugins/Themes/Oxygen2/radialMap/sincos.h

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
    $$PWD/plugins/Listener/catchcopy-v0002/listener.cpp \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ClientCatchcopy.cpp \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ExtraSocketCatchcopy.cpp \
    $$PWD/plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ServerCatchcopy.cpp \
    $$PWD/plugins/Themes/Oxygen2/ThemesFactory.cpp \
    $$PWD/plugins/Themes/Oxygen2/interface.cpp \
    $$PWD/plugins/Themes/Oxygen2/TransferModel.cpp \
    $$PWD/plugins/Themes/Oxygen2/chartarea.cpp \
    $$PWD/plugins/Themes/Oxygen2/fileTree.cpp \
    $$PWD/plugins/Themes/Oxygen2/ProgressBarDark.cpp \
    $$PWD/plugins/Themes/Oxygen2/radialMap/labels.cpp \
    $$PWD/plugins/Themes/Oxygen2/radialMap/map.cpp \
    $$PWD/plugins/Themes/Oxygen2/radialMap/widgetEvents.cpp \
    $$PWD/plugins/Themes/Oxygen2/radialMap/widget.cpp

win32 {
    RESOURCES -= $$PWD/resources/resources-windows-qt-plugin.qrc

    HEADERS         += \
        $$PWD/plugins/PluginLoader/catchcopy-v0002/StructEnumDefinition.h \
        $$PWD/plugins/PluginLoader/catchcopy-v0002/pluginLoader.h \
        $$PWD/plugins/PluginLoader/catchcopy-v0002/DebugEngineMacro.h \
        $$PWD/plugins/PluginLoader/catchcopy-v0002/Environment.h \
        $$PWD/plugins/PluginLoader/catchcopy-v0002/Variable.h \
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
