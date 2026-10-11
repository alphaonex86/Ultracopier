# In-process unit test for ProductKey's online ban-list check (see productkey_test.cpp and
# cases/productkey_ban_check.py). ProductKey.cpp is #included by the test (ban URL -> qrc:),
# ULTRACOPIER_DEBUG routes ULTRACOPIER_DEBUGCONSOLE to a stub that records the check's progress.
QT       += core gui widgets network xml testlib
CONFIG   += console c++17 nodebug
CONFIG   -= app_bundle
TEMPLATE  = app
TARGET    = productkey_test
DEFINES  += ULTRACOPIER_DEBUG

SRCROOT = $$PWD/../../../../..
INCLUDEPATH += $$SRCROOT $$SRCROOT/interface

HEADERS += $$SRCROOT/ProductKey.h $$SRCROOT/OptionEngine.h
SOURCES += $$PWD/productkey_test.cpp
FORMS   += $$SRCROOT/ProductKey.ui
RESOURCES += $$PWD/productkey_test.qrc

# Same warning set as other-pro/ultracopier-core.pro.
QMAKE_CXXFLAGS += -Wall -Wextra -Wconversion
