# In-process FAST test app for the Oxygen theme's end-of-copy window decision (comboBox_copyEnd).
# It links the Oxygen theme exactly as the theme plugin builds it (interface.pri source set) as a
# console app with our own main(). No real copy, no GUI mapping -> runs in milliseconds. See
# window_close_test.cpp and cases/window_close.py.
QT       += core gui widgets xml testlib
CONFIG   += console c++17 nodebug
CONFIG   -= app_bundle
TEMPLATE  = app
TARGET    = window_close_test
DEFINES  += _FILE_OFFSET_BITS=64 UNICODE _UNICODE _LARGE_FILE_SOURCE=1

SRCROOT = $$PWD/../../../../..
OXY     = $$SRCROOT/plugins/Themes/Oxygen
INCLUDEPATH += $$OXY $$SRCROOT $$SRCROOT/interface

HEADERS += $$OXY/interface.h $$OXY/ThemesFactory.h $$OXY/TransferModel.h \
    $$OXY/StructEnumDefinition.h $$OXY/DebugEngineMacro.h $$OXY/Environment.h \
    $$OXY/OxygenVariable.h \
    $$SRCROOT/interface/PluginInterface_Themes.h $$SRCROOT/interface/FacilityInterface.h \
    $$SRCROOT/interface/OptionInterface.h $$SRCROOT/interface/PathTreeStr.h \
    $$SRCROOT/cpp11addition.h

SOURCES += $$PWD/window_close_test.cpp \
    $$OXY/interface.cpp $$OXY/ThemesFactory.cpp $$OXY/TransferModel.cpp \
    $$SRCROOT/interface/PathTreeStr.cpp \
    $$SRCROOT/cpp11addition.cpp $$SRCROOT/cpp11additionstringtointcpp.cpp

FORMS += $$OXY/interface.ui $$OXY/themesOptions.ui

RESOURCES += $$OXY/interfaceResources.qrc \
    $$OXY/interfaceResources_unix.qrc $$OXY/interfaceResources_windows.qrc
