TEMPLATE = subdirs
SUBDIRS = ultracopier copyengine oxygen dbus clean teracopy supercopier windows sessionloader listener
ultracopier.file = ultracopier-core.pro
copyengine.file = plugins/CopyEngine/Ultracopier/copyEngine.pro
oxygen.file = plugins/Themes/Oxygen/interface.pro
listener.file = plugins/Listener/catchcopy-v0002/listener.pro
clean.file = plugins-alternative/Themes/Clean/interface.pro
teracopy.file = plugins-alternative/Themes/Teracopy/interface.pro
supercopier.file = plugins-alternative/Themes/Supercopier/interface.pro
windows.file = plugins-alternative/Themes/Windows/interface.pro
UNAME = $$system(uname -s)
contains( UNAME, [lL]inux ): {
    message( Add Linux ($$UNAME) specific plugin )
    sessionloader.file = plugins/SessionLoader/KDE4/sessionLoader.pro
    SUBDIRS += listener
    dbus.file = plugins/Listener/dbus/listener.pro
}
win32 {
    message( Add Microsoft Windows specific plugin )
    sessionloader.file = plugins/SessionLoader/Windows/sessionLoader.pro
    SUBDIRS += pluginloader
    SUBDIRS -= dbus
    pluginloader.file = plugins/PluginLoader/catchcopy-v0002/pluginLoader.pro
}
