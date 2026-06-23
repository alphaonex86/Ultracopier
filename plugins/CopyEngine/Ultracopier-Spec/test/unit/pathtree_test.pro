# Standalone unit test for the engine's PathTree (test/unit/pathtree_test.cpp).
# Built TWICE by cases/pathtree_unit.py: default (std::string) and DEFINES+=WIDESTRING
# (std::wstring), so both INTERNALTYPEPATH instantiations are covered on Linux.
#
# PathTree.h includes the engine's TransferThread.h only for the INTERNALTYPEPATH /
# INTERNALTYPECHAR typedefs, and that header pulls QtCore declarations -- so we link QtCore
# to let it parse. PathTree.cpp itself uses no Qt symbols, so nothing else is needed.
QT       += core
QT       -= gui
CONFIG   += console c++17
CONFIG   -= app_bundle
TEMPLATE  = app
TARGET    = pathtree_test

SPEC = $$PWD/../..
INCLUDEPATH += $$SPEC $$SPEC/../../..

SOURCES += $$PWD/pathtree_test.cpp \
           $$SPEC/PathTree.cpp
HEADERS += $$SPEC/PathTree.h
# WIDESTRING (std::wstring) is toggled by the caller: the harness builds this .pro once
# plain (std::string) and once with DEFINES+=WIDESTRING (std::wstring).
