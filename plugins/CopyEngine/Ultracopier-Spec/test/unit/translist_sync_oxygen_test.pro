# In-process FAST test app for the OXYGEN transfer-list model (TransferModel::synchronizeItems).
# Links only the model + its path store -- no interface.ui, no engine, no disk. See
# translist_sync_test.cpp and cases/translist_sync.py.
QT       += core gui widgets
CONFIG   += console c++17 nodebug
CONFIG   -= app_bundle
TEMPLATE  = app
TARGET    = translist_sync_oxygen_test
DEFINES  += _FILE_OFFSET_BITS=64 UNICODE _UNICODE _LARGE_FILE_SOURCE=1

SRCROOT = $$PWD/../../../../..
THEME   = $$SRCROOT/plugins/Themes/Oxygen
INCLUDEPATH += $$THEME $$SRCROOT $$SRCROOT/interface

HEADERS += $$THEME/TransferModel.h $$THEME/StructEnumDefinition.h $$THEME/DebugEngineMacro.h \
    $$THEME/Environment.h \
    $$SRCROOT/interface/FacilityInterface.h $$SRCROOT/interface/PathTreeStr.h \
    $$SRCROOT/cpp11addition.h

SOURCES += $$PWD/translist_sync_test.cpp \
    $$THEME/TransferModel.cpp \
    $$SRCROOT/interface/PathTreeStr.cpp \
    $$SRCROOT/cpp11addition.cpp $$SRCROOT/cpp11additionstringtointcpp.cpp

# Same warning set as other-pro/ultracopier-core.pro.
QMAKE_CXXFLAGS += -Wall -Wextra -Wconversion
