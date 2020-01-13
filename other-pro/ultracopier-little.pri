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
    ../plugins/Themes/Oxygen2/ThemesFactory.cpp \
    ../plugins/Themes/Oxygen2/interface.cpp \
    ../plugins/Themes/Oxygen2/TransferModel.cpp \
    ../plugins/Themes/Oxygen2/chartarea.cpp \
    ../plugins/Themes/Oxygen2/fileTree.cpp \
    ../plugins/Themes/Oxygen2/ProgressBarDark.cpp \
    ../plugins/Themes/Oxygen2/DarkButton.cpp \
    ../plugins/Themes/Oxygen2/VerticalLabel.cpp \
    ../plugins/Themes/Oxygen2/radialMap/labels.cpp \
    ../plugins/Themes/Oxygen2/radialMap/map.cpp \
    ../plugins/Themes/Oxygen2/radialMap/widgetEvents.cpp \
    ../plugins/Themes/Oxygen2/radialMap/widget.cpp \
    ../little/OptionsEngineLittle.cpp \
    ../FacilityEngine.cpp \
    ../cpp11addition.cpp \
    ../cpp11additionstringtointcpp.cpp

RESOURCES += \
    ../plugins/Themes/Oxygen2/interfaceResources_unix.qrc \
    ../plugins/Themes/Oxygen2/interfaceResources_windows.qrc \
    ../plugins/Themes/Oxygen2/interfaceResources.qrc

FORMS += \
    ../plugins/Themes/Oxygen2/themesOptions.ui \
    ../plugins/Themes/Oxygen2/options.ui \
    ../plugins/Themes/Oxygen2/interface.ui

DISTFILES +=

HEADERS += \
    ../plugins/Themes/Oxygen2/DebugEngineMacro.h \
    ../plugins/Themes/Oxygen2/Oxygen2Environment.h \
    ../plugins/Themes/Oxygen2/ThemesFactory.h \
    ../plugins/Themes/Oxygen2/interface.h \
    ../plugins/Themes/Oxygen2/Oxygen2Variable.h \
    ../plugins/Themes/Oxygen2/TransferModel.h \
    ../plugins/Themes/Oxygen2/StructEnumDefinition.h \
    ../plugins/Themes/Oxygen2/chartarea.h \
    ../plugins/Themes/Oxygen2/fileTree.h \
    ../plugins/Themes/Oxygen2/ProgressBarDark.h \
    ../plugins/Themes/Oxygen2/DarkButton.h \
    ../plugins/Themes/Oxygen2/VerticalLabel.h \
    ../plugins/Themes/Oxygen2/radialMap/map.h \
    ../plugins/Themes/Oxygen2/radialMap/widget.h \
    ../plugins/Themes/Oxygen2/radialMap/radialMap.h \
    ../little/OptionsEngineLittle.h \
    ../FacilityEngine.h \
    ../interface/FacilityInterface.h \
    ../interface/OptionInterface.h \
    ../interface/PluginInterface_CopyEngine.h \
    ../interface/PluginInterface_SessionLoader.h \
    ../interface/PluginInterface_Themes.h \
    ../interface/PluginInterface_Listener.h \
    ../interface/PluginInterface_PluginLoader.h \
    ../cpp11addition.h
