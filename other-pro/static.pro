DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE

include(ultracopier-core.pro)

RESOURCES += $$PWD/../plugins/static-plugins.qrc \
    $$PWD/../plugins/CopyEngine/Ultracopier/copyEngineResources.qrc

win32:RESOURCES += $$PWD/../plugins/static-plugins-windows.qrc

LIBS           = -Lplugins -lcopyEngine -linterface -llistener
win32:LIBS += -lpluginLoader -lsessionLoader

build_pass:CONFIG(debug, debug|release) {
LIBS           = -Lplugins -lcopyEngined -linterfaced -llistenerd
win32:LIBS += -lpluginLoaderd -lsessionLoaderd
}

HEADERS -= $$PWD/../lib/qt-tar-xz/xz.h \
    $$PWD/../lib/qt-tar-xz/QXzDecodeThread.h \
    $$PWD/../lib/qt-tar-xz/QXzDecode.h \
    $$PWD/../lib/qt-tar-xz/QTarDecode.h \
    $$PWD/../AuthPlugin.h
SOURCES -= $$PWD/../lib/qt-tar-xz/QXzDecodeThread.cpp \
    $$PWD/../lib/qt-tar-xz/QXzDecode.cpp \
    $$PWD/../lib/qt-tar-xz/QTarDecode.cpp \
    $$PWD/../lib/qt-tar-xz/xz_crc32.c \
    $$PWD/../lib/qt-tar-xz/xz_dec_stream.c \
    $$PWD/../lib/qt-tar-xz/xz_dec_lzma2.c \
    $$PWD/../lib/qt-tar-xz/xz_dec_bcj.c \
    $$PWD/../AuthPlugin.cpp
INCLUDEPATH -= $$PWD/../lib/qt-tar-xz/

RESOURCES -= $$PWD/../resources/resources-windows-qt-plugin.qrc
