# Standalone unit test for the theme-side tree storage (interface/PathTreeStr).
# PathTreeStr.h depends only on the C++ standard library (no Qt), so this is a plain
# console app -- no QT modules needed. Built+run by cases/pathtree_unit.py.
QT       -= core gui
CONFIG   += console c++17
CONFIG   -= app_bundle qt
TEMPLATE  = app
TARGET    = pathtreestr_test

IFACE = $$PWD/../../../../../interface
INCLUDEPATH += $$IFACE

SOURCES += $$PWD/pathtreestr_test.cpp \
           $$IFACE/PathTreeStr.cpp
HEADERS += $$IFACE/PathTreeStr.h
