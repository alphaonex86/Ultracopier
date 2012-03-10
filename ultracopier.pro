TEMPLATE = app
QT += network xml
TRANSLATIONS += plugins/Languages/fr/translation.ts \
    plugins/Languages/ar/translation.ts \
    plugins/Languages/ch/translation.ts \
    plugins/Languages/es/translation.ts \
    plugins/Languages/de/translation.ts \
    plugins/Languages/el/translation.ts \
    plugins/Languages/it/translation.ts \
    plugins/Languages/jp/translation.ts \
    plugins/Languages/id/translation.ts \
    plugins/Languages/pl/translation.ts \
    plugins/Languages/ru/translation.ts \
    plugins/Languages/tr/translation.ts \
    plugins/Languages/th/translation.ts \
    plugins/Languages/hi/translation.ts \
    plugins/Languages/nl/translation.ts \
    plugins/Languages/no/translation.ts \
    plugins/Languages/pt/translation.ts

win32:RC_FILE += resources/resources-windows.rc
TARGET = ultracopier
macx { 
    ICON = resources/ultracopier.icns
}
FORMS += HelpDialog.ui \
    PluginInformation.ui \
    OptionDialog.ui
RESOURCES += resources/resources.qrc
win32 {
    RESOURCES += resources/resources_windows.qrc
}
!win32 {
    RESOURCES += resources/resources_unix.qrc
}

HEADERS += ResourcesManager.h \
    ThemesManager.h \
    SystrayIcon.h \
    StructEnumDefinition.h \
    Singleton.h \
    EventDispatcher.h \
    Environment.h \
    DebugEngine.h \
    Core.h \
    GlobalClass.h \
    OptionEngine.h \
    HelpDialog.h \
    PluginsManager.h \
    LanguagesManager.h \
    DebugEngineMacro.h \
    PluginInformation.h \
    lib/qt-tar-xz/xz.h \
    lib/qt-tar-xz/QXzDecodeThread.h \
    lib/qt-tar-xz/QXzDecode.h \
    lib/qt-tar-xz/QTarDecode.h \
    AuthPlugin.h \
    SessionLoader.h \
    ExtraSocket.h \
    CopyListener.h \
    CopyEngineManager.h \
    PlatformMacro.h \
    interface/PluginInterface_Themes.h \
    interface/PluginInterface_SessionLoader.h \
    interface/PluginInterface_Listener.h \
    interface/PluginInterface_CopyEngine.h \
    interface/OptionInterface.h \
    Variable.h \
    PluginLoader.h \
    interface/PluginInterface_PluginLoader.h \
    OptionDialog.h \
    LocalPluginOptions.h \
    LocalListener.h \
    CliParser.h \
    interface/FacilityInterface.h \
    FacilityEngine.h \
    LogThread.h \
    CompilerInfo.h \
    StructEnumDefinition_UltracopierSpecific.h
SOURCES += ThemesManager.cpp \
    ResourcesManager.cpp \
    main.cpp \
    EventDispatcher.cpp \
    SystrayIcon.cpp \
    GlobalClass.cpp \
    DebugEngine.cpp \
    OptionEngine.cpp \
    HelpDialog.cpp \
    PluginsManager.cpp \
    LanguagesManager.cpp \
    PluginInformation.cpp \
    lib/qt-tar-xz/QXzDecodeThread.cpp \
    lib/qt-tar-xz/QXzDecode.cpp \
    lib/qt-tar-xz/QTarDecode.cpp \
    lib/qt-tar-xz/xz_crc32.c \
    lib/qt-tar-xz/xz_dec_stream.c \
    lib/qt-tar-xz/xz_dec_lzma2.c \
    lib/qt-tar-xz/xz_dec_bcj.c \
    AuthPlugin.cpp \
    SessionLoader.cpp \
    ExtraSocket.cpp \
    CopyListener.cpp \
    CopyEngineManager.cpp \
    Core.cpp \
    PluginLoader.cpp \
    OptionDialog.cpp \
    LocalPluginOptions.cpp \
    LocalListener.cpp \
    CliParser.cpp \
    FacilityEngine.cpp \
    LogThread.cpp
INCLUDEPATH += lib/qt-tar-xz/

OTHER_FILES += \
    resources/resources-windows.rc

