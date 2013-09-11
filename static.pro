DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE

include(ultracopier-core.pro)

RESOURCES += plugins/static-plugins.qrc \
    plugins/CopyEngine/Ultracopier/copyEngineResources.qrc

win32:RESOURCES += plugins/static-plugins-windows.qrc

LIBS           = -Lplugins -lcopyEngine -linterface -llistener -lQt5SystemInfo
win32:LIBS += -lpluginLoader -lsessionLoader -lPowrProf -lSetupapi

build_pass:CONFIG(debug, debug|release) {
LIBS           = -Lplugins -lcopyEngined -linterfaced -llistenerd -lQt5SystemInfod
win32:LIBS += -lpluginLoaderd -lsessionLoaderd -lPowrProf -lSetupapi
}

HEADERS -= lib/qt-tar-xz/xz.h \
    lib/qt-tar-xz/QXzDecodeThread.h \
    lib/qt-tar-xz/QXzDecode.h \
    lib/qt-tar-xz/QTarDecode.h \
    AuthPlugin.h
SOURCES -= lib/qt-tar-xz/QXzDecodeThread.cpp \
    lib/qt-tar-xz/QXzDecode.cpp \
    lib/qt-tar-xz/QTarDecode.cpp \
    lib/qt-tar-xz/xz_crc32.c \
    lib/qt-tar-xz/xz_dec_stream.c \
    lib/qt-tar-xz/xz_dec_lzma2.c \
    lib/qt-tar-xz/xz_dec_bcj.c \
    AuthPlugin.cpp
INCLUDEPATH -= lib/qt-tar-xz/

RESOURCES -= resources/resources-windows-qt-plugin.qrc
