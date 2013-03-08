DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE

include(ultracopier-core.pro)

RESOURCES += plugins/static-plugins.qrc \
    plugins/CopyEngine/Ultracopier/copyEngineResources.qrc \
    plugins/Themes/Oxygen/interfaceResources_windows.qrc \
    plugins/Themes/Oxygen/interfaceResources_unix.qrc \
    plugins/Themes/Oxygen/interfaceResources.qrc

LIBS           = -Lplugins -lcopyEngine -linterface -llistener

win32:LIBS += -lpluginLoader -lsessionLoader

if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
   mac:LIBS = $$member(LIBS, 0) $$member(LIBS, 1)_debug
   win32:LIBS = $$member(LIBS, 0) $$member(LIBS, 1)d
}
