QT       -= core gui

CONFIG += 64bit
CONFIG -= exceptions rtti

#DEFINES += CATCHCOPY_EXPLORER_PLUGIN_DEBUG

CONFIG(32bit) {
    TARGET = catchcopy32
    QMAKE_CFLAGS =  -march=i586 -m32
        QMAKE_CXXFLAGS = -march=i586 -m32
        QMAKE_LFLAGS += -m32
}
CONFIG(64bit) {
    TARGET = catchcopy64
    LIBS += -LC:\x86_64-4.9.3-release-win32-seh-rt_v4-rev1\mingw64\lib64
        DEFINES += _M_X64
        QMAKE_CFLAGS = -m64
        QMAKE_CXXFLAGS = -m64
        QMAKE_LFLAGS += -m64
}

QMAKE_CFLAGS -= -fexceptions -mthreads -O2
QMAKE_CXXFLAGS -= -fexceptions -mthreads -O2
QMAKE_CFLAGS += -std=c++98 -fno-keep-inline-dllexport -mtune=generic -fno-exceptions -Os -Wall -Wextra -fno-rtti -s -static -static-libgcc -static-libstdc++
QMAKE_CXXFLAGS += -std=c++98 -fno-keep-inline-dllexport -mtune=generic -fno-exceptions -Os -Wall -Wno-write-strings -Wextra -fno-rtti -s -static -static-libgcc -static-libstdc++

DEF_FILE += CatchCopy.def

LIBS+= -lws2_32 -lole32 -luuid -static-libstdc++ -static-libgcc -static

TEMPLATE = lib

HEADERS += \
    Variable.h \
    Deque.h \
    resource.h \
    Reg.h \
    ClientCatchcopy.h \
    DDShellExt.h \
    ClassFactory.h

SOURCES += \
    Deque.cpp \
    ClientCatchcopy.cpp \
    Reg.cpp \
    DDShellExt.cpp \
    ClassFactory.cpp \
    CatchCopy.cpp
