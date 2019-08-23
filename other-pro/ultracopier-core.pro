CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x -Wall -Wextra"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"
#QMAKE_CXXFLAGS+="-Wall -Wextra -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-macros -Wno-newline-eof -Wno-exit-time-destructors -Wno-global-constructors -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation -Wno-shadow -Wno-missing-prototypes -Wno-padded -Wno-covered-switch-default -Wno-old-style-cast -Wno-documentation-unknown-command -Wno-switch-enum -Wno-undefined-reinterpret-cast -Wno-unreachable-code-break -Wno-sign-conversion -Wno-float-conversion"
QMAKE_CXXFLAGS+=""
DEFINES += _LARGE_FILE_SOURCE=1 _FILE_OFFSET_BITS=64 _UNICODE UNICODE

wasm: DEFINES += NOAUDIO
macx {
    DEFINES += NOAUDIO
    LIBS += /usr/local/Cellar/opus/1.3.1/lib/libopus.a
    INCLUDEPATH += /usr/local/Cellar/opus/1.3.1/include/
}
#DEFINES += NOAUDIO
!contains(DEFINES, NOAUDIO) {
QT += multimedia
linux:LIBS += -lopus
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

TEMPLATE = app
QT += network xml widgets
TRANSLATIONS += $$PWD/../plugins/Languages/ar/translation.ts \
    $$PWD/../plugins/Languages/de/translation.ts \
    $$PWD/../plugins/Languages/el/translation.ts \
    $$PWD/../resources/Languages/en/translation.ts \
    $$PWD/../plugins/Languages/es/translation.ts \
    $$PWD/../plugins/Languages/fr/translation.ts \
    $$PWD/../plugins/Languages/hi/translation.ts \
    $$PWD/../plugins/Languages/hu/translation.ts \
    $$PWD/../plugins/Languages/id/translation.ts \
    $$PWD/../plugins/Languages/it/translation.ts \
    $$PWD/../plugins/Languages/ja/translation.ts \
    $$PWD/../plugins/Languages/ko/translation.ts \
    $$PWD/../plugins/Languages/nl/translation.ts \
    $$PWD/../plugins/Languages/no/translation.ts \
    $$PWD/../plugins/Languages/pl/translation.ts \
    $$PWD/../plugins/Languages/pt/translation.ts \
    $$PWD/../plugins/Languages/ru/translation.ts \
    $$PWD/../plugins/Languages/th/translation.ts \
    $$PWD/../plugins/Languages/tr/translation.ts \
    $$PWD/../plugins/Languages/zh/translation.ts \
    $$PWD/../plugins/Languages/zh_TW/translation.ts

TARGET = ultracopier
macx {
    ICON = $$PWD/../resources/ultracopier.icns
    #QT += macextras
    VERSION = 2.0.0.1
}
FORMS += $$PWD/../HelpDialog.ui \
    $$PWD/../PluginInformation.ui \
    $$PWD/../OptionDialog.ui \
    $$PWD/../OSSpecific.ui \
    $$PWD/../ProductKey.ui
RESOURCES += \
    $$PWD/../resources/ultracopier-resources.qrc \
    $$PWD/../resources/ultracopier-resources_unix.qrc \
    $$PWD/../resources/ultracopier-resources_windows.qrc
win32 {
    !contains(DEFINES, ULTRACOPIER_PLUGIN_ALL_IN_ONE) {
        RESOURCES += $$PWD/../resources/resources-windows-qt-plugin.qrc
    }
    RC_FILE += $$PWD/../resources/resources-windows.rc
    #LIBS += -lpdh
        LIBS += -ladvapi32
}

HEADERS += $$PWD/../ResourcesManager.h \
    $$PWD/../ThemesManager.h \
    $$PWD/../SystrayIcon.h \
    $$PWD/../StructEnumDefinition.h \
    $$PWD/../EventDispatcher.h \
    $$PWD/../Environment.h \
    $$PWD/../DebugEngine.h \
    $$PWD/../Core.h \
    $$PWD/../OptionEngine.h \
    $$PWD/../HelpDialog.h \
    $$PWD/../PluginsManager.h \
    $$PWD/../LanguagesManager.h \
    $$PWD/../DebugEngineMacro.h \
    $$PWD/../PluginInformation.h \
    $$PWD/../lib/qt-tar-xz/xz.h \
    $$PWD/../lib/qt-tar-xz/QXzDecodeThread.h \
    $$PWD/../lib/qt-tar-xz/QXzDecode.h \
    $$PWD/../lib/qt-tar-xz/QTarDecode.h \
    $$PWD/../SessionLoader.h \
    $$PWD/../ExtraSocket.h \
    $$PWD/../CopyListener.h \
    $$PWD/../CopyEngineManager.h \
    $$PWD/../PlatformMacro.h \
    $$PWD/../interface/PluginInterface_Themes.h \
    $$PWD/../interface/PluginInterface_SessionLoader.h \
    $$PWD/../interface/PluginInterface_Listener.h \
    $$PWD/../interface/PluginInterface_CopyEngine.h \
    $$PWD/../interface/OptionInterface.h \
    $$PWD/../Variable.h \
    $$PWD/../Version.h \
    $$PWD/../PluginLoaderCore.h \
    $$PWD/../interface/PluginInterface_PluginLoader.h \
    $$PWD/../OptionDialog.h \
    $$PWD/../LocalPluginOptions.h \
    $$PWD/../LocalListener.h \
    $$PWD/../CliParser.h \
    $$PWD/../interface/FacilityInterface.h \
    $$PWD/../FacilityEngine.h \
    $$PWD/../LogThread.h \
    $$PWD/../CompilerInfo.h \
    $$PWD/../StructEnumDefinition_UltracopierSpecific.h \
    $$PWD/../OSSpecific.h \
    $$PWD/../cpp11addition.h \
    $$PWD/../InternetUpdater.h \
    $$PWD/../ProductKey.h
SOURCES += $$PWD/../ThemesManager.cpp \
    $$PWD/../ResourcesManager.cpp \
    $$PWD/../main.cpp \
    $$PWD/../EventDispatcher.cpp \
    $$PWD/../SystrayIcon.cpp \
    $$PWD/../DebugEngine.cpp \
    $$PWD/../OptionEngine.cpp \
    $$PWD/../HelpDialog.cpp \
    $$PWD/../PluginsManager.cpp \
    $$PWD/../LanguagesManager.cpp \
    $$PWD/../PluginInformation.cpp \
    $$PWD/../lib/qt-tar-xz/QXzDecodeThread.cpp \
    $$PWD/../lib/qt-tar-xz/QXzDecode.cpp \
    $$PWD/../lib/qt-tar-xz/QTarDecode.cpp \
    $$PWD/../lib/qt-tar-xz/xz_crc32.c \
    $$PWD/../lib/qt-tar-xz/xz_dec_stream.c \
    $$PWD/../lib/qt-tar-xz/xz_dec_lzma2.c \
    $$PWD/../lib/qt-tar-xz/xz_dec_bcj.c \
    $$PWD/../SessionLoader.cpp \
    $$PWD/../ExtraSocket.cpp \
    $$PWD/../CopyListener.cpp \
    $$PWD/../CopyEngineManager.cpp \
    $$PWD/../Core.cpp \
    $$PWD/../PluginLoaderCore.cpp \
    $$PWD/../OptionDialog.cpp \
    $$PWD/../LocalPluginOptions.cpp \
    $$PWD/../LocalListener.cpp \
    $$PWD/../CliParser.cpp \
    $$PWD/../FacilityEngine.cpp \
    $$PWD/../FacilityEngineVersion.cpp \
    $$PWD/../LogThread.cpp \
    $$PWD/../OSSpecific.cpp \
    $$PWD/../cpp11addition.cpp \
    $$PWD/../DebugModel.cpp \
    $$PWD/../InternetUpdater.cpp \
    $$PWD/../cpp11additionstringtointcpp.cpp \
    $$PWD/../ProductKey.cpp
INCLUDEPATH += \
    $$PWD/../lib/qt-tar-xz/

OTHER_FILES += $$PWD/../resources/resources-windows.rc

win32: {
DEFINES += WIDESTRING
QT += winextras
}
DEFINES += WIDESTRING
