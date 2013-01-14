#-------------------------------------------------
#
# Project created by QtCreator 2012-10-19T22:52:40
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = unit-tester
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    ../../plugins/CopyEngine/Ultracopier/ReadThread.cpp \
    ../../plugins/CopyEngine/Ultracopier/AvancedQFile.cpp \
    ../../plugins/CopyEngine/Ultracopier/WriteThread.cpp \
    ../../plugins/CopyEngine/Ultracopier/TransferThread.cpp \
    ../../plugins/CopyEngine/Ultracopier/ListThread_InodeAction.cpp \
    ../../plugins/CopyEngine/Ultracopier/ListThread.cpp \
    ../../plugins/CopyEngine/Ultracopier/MkPath.cpp \
    ../../plugins/CopyEngine/Ultracopier/scanFileOrFolder.cpp \
    ../../plugins/CopyEngine/Ultracopier/RmPath.cpp \
    copyEngineUnitTester.cpp \
    copyEngine.cpp

HEADERS += \
    ../../plugins/CopyEngine/Ultracopier/ReadThread.h \
    ../../plugins/CopyEngine/Ultracopier/AvancedQFile.h \
    ../../plugins/CopyEngine/Ultracopier/Variable.h \
    ../../plugins/CopyEngine/Ultracopier/WriteThread.h \
    ../../plugins/CopyEngine/Ultracopier/TransferThread.h \
    ../../plugins/CopyEngine/Ultracopier/ListThread.h \
    ../../plugins/CopyEngine/Ultracopier/MkPath.h \
    ../../plugins/CopyEngine/Ultracopier/scanFileOrFolder.h \
    ../../plugins/CopyEngine/Ultracopier/RmPath.h \
    copyEngineUnitTester.h \
    copyEngine.h
