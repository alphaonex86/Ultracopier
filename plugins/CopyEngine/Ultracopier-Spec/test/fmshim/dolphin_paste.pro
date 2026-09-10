# Builds the REAL patched DolphinView::pasteToUrl() against the pinned Dolphin/KIO stub, so the
# file-manager patch can be executed against a live Ultracopier. The patched function itself is
# NOT in the repo: cases/dolphin_paste_patch.py extracts it from
# file-manager/dolphin-0002-*.patch into a generated .cpp and passes it on the qmake line:
#   qmake6 -o Makefile dolphin_paste.pro "SOURCES+=<generated>/dolphin_paste_patched.cpp"
QT       += core gui widgets network
CONFIG   += console c++17
CONFIG   -= app_bundle
TEMPLATE  = app
TARGET    = dolphin_paste_driver

INCLUDEPATH += $$PWD
HEADERS += $$PWD/dolphin_api_stub.h
SOURCES += $$PWD/dolphin_paste_driver.cpp

QMAKE_CXXFLAGS += -Wall -Wextra
