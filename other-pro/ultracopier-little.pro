CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"
#QMAKE_CXXFLAGS+="-Wall -Wextra -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-macros -Wno-newline-eof -Wno-exit-time-destructors -Wno-global-constructors -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation -Wno-shadow -Wno-missing-prototypes -Wno-padded -Wno-covered-switch-default -Wno-old-style-cast -Wno-documentation-unknown-command -Wno-switch-enum -Wno-undefined-reinterpret-cast -Wno-unreachable-code-break -Wno-sign-conversion -Wno-float-conversion"

DEFINES += ULTRACOPIER_LITTLE ULTRACOPIER_PLUGIN_ALL_IN_ONE ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT ULTRACOPIER_NODEBUG
DEFINES -= ULTRACOPIER_DEBUG

TEMPLATE = app
QT += widgets

TARGET = ultracopier
macx {
    ICON = $$PWD/../resources/ultracopier.icns
    #QT += macextras
}
win32 {
    RC_FILE += $$PWD/../resources/resources-windows.rc
    LIBS += -ladvapi32
}

OTHER_FILES += $$PWD/../resources/resources-windows.rc

SOURCES += \
    ../little/main-little.cpp \
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
    ../plugins/Themes/Oxygen2/ThemesFactory.cpp \
    ../plugins/Themes/Oxygen2/interface.cpp \
    ../plugins/Themes/Oxygen2/TransferModel.cpp \
    ../plugins/Themes/Oxygen2/chartarea.cpp \
    ../plugins/Themes/Oxygen2/fileTree.cpp \
    ../plugins/Themes/Oxygen2/ProgressBarDark.cpp \
    ../plugins/Themes/Oxygen2/DarkButton.cpp \
    ../plugins/Themes/Oxygen2/radialMap/labels.cpp \
    ../plugins/Themes/Oxygen2/radialMap/map.cpp \
    ../plugins/Themes/Oxygen2/radialMap/widgetEvents.cpp \
    ../plugins/Themes/Oxygen2/radialMap/widget.cpp \
    ../little/OptionsEngineLittle.cpp \
    ../FacilityEngine.cpp \
    ../cpp11addition.cpp \
    ../cpp11additionstringtointcpp.cpp

RESOURCES += \
    ../plugins/CopyEngine/Ultracopier-Spec/copyEngineResources.qrc \
    ../plugins/Themes/Oxygen2/interfaceResources_unix.qrc \
    ../plugins/Themes/Oxygen2/interfaceResources_windows.qrc \
    ../plugins/Themes/Oxygen2/interfaceResources.qrc

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
    ../plugins/CopyEngine/Ultracopier-Spec/RenamingRules.ui \
    ../plugins/Themes/Oxygen2/themesOptions.ui \
    ../plugins/Themes/Oxygen2/options.ui \
    ../plugins/Themes/Oxygen2/interface.ui

DISTFILES +=

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
    ../plugins/CopyEngine/Ultracopier-Spec/Variable.h \
    ../plugins/CopyEngine/Ultracopier-Spec/async/TransferThreadAsync.h \
    ../plugins/Themes/Oxygen2/DebugEngineMacro.h \
    ../plugins/Themes/Oxygen2/Environment.h \
    ../plugins/Themes/Oxygen2/ThemesFactory.h \
    ../plugins/Themes/Oxygen2/interface.h \
    ../plugins/Themes/Oxygen2/Variable.h \
    ../plugins/Themes/Oxygen2/TransferModel.h \
    ../plugins/Themes/Oxygen2/StructEnumDefinition.h \
    ../plugins/Themes/Oxygen2/chartarea.h \
    ../plugins/Themes/Oxygen2/fileTree.h \
    ../plugins/Themes/Oxygen2/ProgressBarDark.h \
    ../plugins/Themes/Oxygen2/DarkButton.h \
    ../plugins/Themes/Oxygen2/radialMap/map.h \
    ../plugins/Themes/Oxygen2/radialMap/widget.h \
    ../plugins/Themes/Oxygen2/radialMap/radialMap.h \
    ../little/OptionsEngineLittle.h \
    ../FacilityEngine.h \
    ../Variable.h \
    ../interface/FacilityInterface.h \
    ../interface/OptionInterface.h \
    ../interface/PluginInterface_CopyEngine.h \
    ../interface/PluginInterface_SessionLoader.h \
    ../interface/PluginInterface_Themes.h \
    ../interface/PluginInterface_Listener.h \
    ../interface/PluginInterface_PluginLoader.h \
    ../cpp11addition.h
