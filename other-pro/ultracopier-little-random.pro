include(ultracopier-little.pri)

DEFINES += ULTRACOPIER_LITTLE_RANDOM NOAUDIO

SOURCES += \
    ../plugins/CopyEngine/Random/CopyEngine.cpp \
    ../plugins/CopyEngine/Random/CopyEngineFactory.cpp

HEADERS += \
    ../plugins/CopyEngine/Random/CopyEngine.h \
    ../plugins/CopyEngine/Random/CopyEngineFactory.h
