# Standalone unit test for extractProtocol() (sources/cpp11addition.cpp) -- the source/destination
# protocol classifier Core refuses unsupported transfers with. cpp11addition.cpp needs no Qt, so
# this is a plain console app. Built+run on Linux by cases/protocol_unit.py.
QT       -= core gui
CONFIG   += console c++17
CONFIG   -= app_bundle qt
TEMPLATE  = app
TARGET    = protocol_test

SOURCES_ROOT = $$PWD/../../../../..
INCLUDEPATH += $$SOURCES_ROOT

SOURCES += $$PWD/protocol_test.cpp \
           $$SOURCES_ROOT/cpp11addition.cpp \
           $$SOURCES_ROOT/cpp11additionstringtointcpp.cpp
HEADERS += $$SOURCES_ROOT/cpp11addition.h

# Same warning set as other-pro/ultracopier-core.pro.
QMAKE_CXXFLAGS += -Wall -Wextra -Wconversion
