DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE

include(ultracopier-core.pro)

RESOURCES += plugins/static-plugins.qrc \
    plugins/CopyEngine/Ultracopier/copyEngineResources.qrc \
    plugins/Themes/Oxygen/interfaceResources_windows.qrc \
    plugins/Themes/Oxygen/interfaceResources_unix.qrc \
    plugins/Themes/Oxygen/interfaceResources.qrc

win32:RESOURCES += plugins/static-plugins-windows.qrc

LIBS           = -Lplugins -lcopyEngine -linterface -llistener -lQt5SystemInfo
win32:LIBS += -lpluginLoader -lsessionLoader

build_pass:CONFIG(debug, debug|release) {
LIBS           = -Lplugins -lcopyEngined -linterfaced -llistenerd -lQt5SystemInfod
win32:LIBS += -lpluginLoaderd -lsessionLoaderd
}
