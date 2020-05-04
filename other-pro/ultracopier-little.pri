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
    ../plugins/Themes/Oxygen/interface.cpp \
    ../plugins/Themes/Oxygen/ThemesFactory.cpp \
    ../plugins/Themes/Oxygen/TransferModel.cpp \
    ../little/OptionsEngineLittle.cpp \
    ../FacilityEngine.cpp \
    ../cpp11addition.cpp \
    ../cpp11additionstringtointcpp.cpp

RESOURCES += \
    ../plugins/Themes/Oxygen/interfaceResources_unix.qrc \
    ../plugins/Themes/Oxygen/interfaceResources_windows.qrc \
    ../plugins/Themes/Oxygen/interfaceResources.qrc

FORMS += \
    ../plugins/Themes/Oxygen/themesOptions.ui \
    ../plugins/Themes/Oxygen/options.ui \
    ../plugins/Themes/Oxygen/interface.ui

DISTFILES +=

HEADERS += \
    ../plugins/Themes/Oxygen/DebugEngineMacro.h \
    ../plugins/Themes/Oxygen/Environment.h \
    ../plugins/Themes/Oxygen/interface.h \
    ../plugins/Themes/Oxygen/OxygenVariable.h \
    ../plugins/Themes/Oxygen/StructEnumDefinition.h \
    ../plugins/Themes/Oxygen/ThemesFactory.h \
    ../plugins/Themes/Oxygen/TransferModel.h \
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
