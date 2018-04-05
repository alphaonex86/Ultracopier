CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"
#QMAKE_CXXFLAGS+="-Wall -Wextra -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-macros -Wno-newline-eof -Wno-exit-time-destructors -Wno-global-constructors -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation -Wno-shadow -Wno-missing-prototypes -Wno-padded -Wno-covered-switch-default -Wno-old-style-cast -Wno-documentation-unknown-command -Wno-switch-enum -Wno-undefined-reinterpret-cast -Wno-unreachable-code-break -Wno-sign-conversion -Wno-float-conversion"

DEFINES += ULTRACOPIER_LITTLE ULTRACOPIER_PLUGIN_ALL_IN_ONE ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT

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
    ../plugins/CopyEngine/Ultracopier/AvancedQFile.cpp \
    ../plugins/CopyEngine/Ultracopier/TransferThread.cpp \
    ../plugins/CopyEngine/Ultracopier/WriteThread.cpp \
    ../plugins/CopyEngine/Ultracopier/ReadThread.cpp \
    ../plugins/CopyEngine/Ultracopier/RenamingRules.cpp \
    ../plugins/CopyEngine/Ultracopier/ScanFileOrFolder.cpp \
    ../plugins/CopyEngine/Ultracopier/CopyEngine-collision-and-error.cpp \
    ../plugins/CopyEngine/Ultracopier/CopyEngine.cpp \
    ../plugins/CopyEngine/Ultracopier/CopyEngineFactory.cpp \
    ../plugins/CopyEngine/Ultracopier/DebugDialog.cpp \
    ../plugins/CopyEngine/Ultracopier/DiskSpace.cpp \
    ../plugins/CopyEngine/Ultracopier/DriveManagement.cpp \
    ../plugins/CopyEngine/Ultracopier/FileErrorDialog.cpp \
    ../plugins/CopyEngine/Ultracopier/FileExistsDialog.cpp \
    ../plugins/CopyEngine/Ultracopier/FileIsSameDialog.cpp \
    ../plugins/CopyEngine/Ultracopier/FilterRules.cpp \
    ../plugins/CopyEngine/Ultracopier/Filters.cpp \
    ../plugins/CopyEngine/Ultracopier/FolderExistsDialog.cpp \
    ../plugins/CopyEngine/Ultracopier/ListThread_InodeAction.cpp \
    ../plugins/CopyEngine/Ultracopier/ListThread.cpp \
    ../plugins/CopyEngine/Ultracopier/MkPath.cpp \
    ../plugins/Themes/Oxygen/ThemesFactory.cpp \
    ../plugins/Themes/Oxygen/TransferModel.cpp \
    ../plugins/Themes/Oxygen/interface.cpp \
    OptionsEngineLittle.cpp \
    ../FacilityEngine.cpp

RESOURCES += \
    ../plugins/CopyEngine/Ultracopier/copyEngineResources.qrc \
    ../plugins/Themes/Oxygen/interfaceResources_unix.qrc \
    ../plugins/Themes/Oxygen/interfaceResources_windows.qrc \
    ../plugins/Themes/Oxygen/interfaceResources.qrc

FORMS += \
    ../plugins/CopyEngine/Ultracopier/RenamingRules.ui \
    ../plugins/CopyEngine/Ultracopier/copyEngineOptions.ui \
    ../plugins/CopyEngine/Ultracopier/debugDialog.ui \
    ../plugins/CopyEngine/Ultracopier/DiskSpace.ui \
    ../plugins/CopyEngine/Ultracopier/fileErrorDialog.ui \
    ../plugins/CopyEngine/Ultracopier/fileExistsDialog.ui \
    ../plugins/CopyEngine/Ultracopier/fileIsSameDialog.ui \
    ../plugins/CopyEngine/Ultracopier/FilterRules.ui \
    ../plugins/CopyEngine/Ultracopier/Filters.ui \
    ../plugins/CopyEngine/Ultracopier/folderExistsDialog.ui \
    ../plugins/Themes/Oxygen/interface.ui \
    ../plugins/Themes/Oxygen/themesOptions.ui \
    ../plugins/Themes/Oxygen/options.ui

DISTFILES +=

HEADERS += \
    ../plugins/CopyEngine/Ultracopier/AvancedQFile.h \
    ../plugins/CopyEngine/Ultracopier/CompilerInfo.h \
    ../plugins/CopyEngine/Ultracopier/TransferThread.h \
    ../plugins/CopyEngine/Ultracopier/Variable.h \
    ../plugins/CopyEngine/Ultracopier/WriteThread.h \
    ../plugins/CopyEngine/Ultracopier/ReadThread.h \
    ../plugins/CopyEngine/Ultracopier/RenamingRules.h \
    ../plugins/CopyEngine/Ultracopier/ScanFileOrFolder.h \
    ../plugins/CopyEngine/Ultracopier/StructEnumDefinition_CopyEngine.h \
    ../plugins/CopyEngine/Ultracopier/StructEnumDefinition.h \
    ../plugins/CopyEngine/Ultracopier/CopyEngine.h \
    ../plugins/CopyEngine/Ultracopier/CopyEngineFactory.h \
    ../plugins/CopyEngine/Ultracopier/DebugDialog.h \
    ../plugins/CopyEngine/Ultracopier/DebugEngineMacro.h \
    ../plugins/CopyEngine/Ultracopier/DiskSpace.h \
    ../plugins/CopyEngine/Ultracopier/DriveManagement.h \
    ../plugins/CopyEngine/Ultracopier/Environment.h \
    ../plugins/CopyEngine/Ultracopier/FileErrorDialog.h \
    ../plugins/CopyEngine/Ultracopier/FileExistsDialog.h \
    ../plugins/CopyEngine/Ultracopier/FileIsSameDialog.h \
    ../plugins/CopyEngine/Ultracopier/FilterRules.h \
    ../plugins/CopyEngine/Ultracopier/Filters.h \
    ../plugins/CopyEngine/Ultracopier/FolderExistsDialog.h \
    ../plugins/CopyEngine/Ultracopier/ListThread.h \
    ../plugins/CopyEngine/Ultracopier/MkPath.h \
    ../plugins/Themes/Oxygen/ThemesFactory.h \
    ../plugins/Themes/Oxygen/TransferModel.h \
    ../plugins/Themes/Oxygen/Variable.h \
    ../plugins/Themes/Oxygen/StructEnumDefinition.h \
    ../plugins/Themes/Oxygen/interface.h \
    ../plugins/Themes/Oxygen/DebugEngineMacro.h \
    ../plugins/Themes/Oxygen/Environment.h \
    OptionsEngineLittle.h \
    ../FacilityEngine.h \
    ../Variable.h
