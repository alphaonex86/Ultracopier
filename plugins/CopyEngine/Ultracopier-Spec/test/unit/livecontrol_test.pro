# In-process FAST test app for the engine, driven HEADLESSLY (no window, no real copy, no Qt
# event loop unless a test needs one). It links the WHOLE Ultracopier-Spec engine -- the list
# logic is monolithically coupled (ListThread -> async TransferThread -> collision code ->
# CopyEngine + the file dialogs), so a "headless list test" still has to link the full engine.
# This .pro is therefore the reusable foundation for ALL fast in-process tests (#18):
# reconciliation/reorder + scale today; the PluginInterface_CopyEngine API and corruption tests
# next. It mirrors CopyEngine.pro's source set as a console app with our own main().
QT       += core gui widgets xml
CONFIG   += console c++17 nodebug
CONFIG   -= app_bundle
TEMPLATE  = app
TARGET    = livecontrol_test
DEFINES  += _FILE_OFFSET_BITS=64 UNICODE _UNICODE _LARGE_FILE_SOURCE=1

SPEC = $$PWD/../..
INCLUDEPATH += $$SPEC $$SPEC/../../..

FORMS += $$SPEC/copyEngineOptions.ui $$SPEC/debugDialog.ui $$SPEC/DiskSpace.ui \
         $$SPEC/fileErrorDialog.ui $$SPEC/fileExistsDialog.ui $$SPEC/fileIsSameDialog.ui \
         $$SPEC/FilterRules.ui $$SPEC/Filters.ui $$SPEC/folderExistsDialog.ui $$SPEC/RenamingRules.ui
RESOURCES += $$SPEC/copyEngineResources.qrc

HEADERS += $$SPEC/StructEnumDefinition.h $$SPEC/StructEnumDefinition_CopyEngine.h \
    $$SPEC/TransferThread.h $$SPEC/MkPath.h $$SPEC/PathTree.h $$SPEC/ListThread.h \
    $$SPEC/../../../interface/PluginInterface_CopyEngine.h $$SPEC/../../../interface/OptionInterface.h \
    $$SPEC/../../../interface/FacilityInterface.h $$SPEC/../../../cpp11addition.h \
    $$SPEC/Filters.h $$SPEC/FilterRules.h $$SPEC/RenamingRules.h $$SPEC/DriveManagement.h \
    $$SPEC/CopyEngine.h $$SPEC/DebugDialog.h $$SPEC/CopyEngineFactory.h $$SPEC/FileErrorDialog.h \
    $$SPEC/FileExistsDialog.h $$SPEC/FileIsSameDialog.h $$SPEC/FolderExistsDialog.h \
    $$SPEC/ScanFileOrFolder.h $$SPEC/DiskSpace.h \
    $$SPEC/async/ReadThread.h $$SPEC/async/WriteThread.h $$SPEC/async/TransferThreadAsync.h

SOURCES += $$PWD/livecontrol_test.cpp \
    $$SPEC/TransferThread.cpp $$SPEC/MkPath.cpp $$SPEC/PathTree.cpp $$SPEC/ListThread.cpp \
    $$SPEC/../../../cpp11addition.cpp $$SPEC/../../../cpp11additionstringtointcpp.cpp \
    $$SPEC/Filters.cpp $$SPEC/FilterRules.cpp $$SPEC/RenamingRules.cpp $$SPEC/ListThread_InodeAction.cpp \
    $$SPEC/DriveManagement.cpp $$SPEC/CopyEngine-collision-and-error.cpp $$SPEC/CopyEngine.cpp \
    $$SPEC/DebugDialog.cpp $$SPEC/CopyEngineFactory.cpp $$SPEC/FileErrorDialog.cpp \
    $$SPEC/FileExistsDialog.cpp $$SPEC/FileIsSameDialog.cpp $$SPEC/FolderExistsDialog.cpp \
    $$SPEC/ScanFileOrFolder.cpp $$SPEC/DiskSpace.cpp $$SPEC/ListThreadActions.cpp \
    $$SPEC/ListThreadListChange.cpp $$SPEC/ListThreadMedia.cpp $$SPEC/ListThreadNew.cpp \
    $$SPEC/ListThreadOptions.cpp $$SPEC/ListThreadScan.cpp $$SPEC/ListThreadStat.cpp \
    $$SPEC/async/ReadThread.cpp $$SPEC/async/WriteThread.cpp $$SPEC/async/TransferThreadAsync.cpp

# xxhash: system header if present, else the bundled vendor copy (mirrors CopyEngine.pro).
XXHASH_USE_VENDOR = 1
exists(/usr/include/xxhash.h):       XXHASH_USE_VENDOR = 0
exists(/usr/local/include/xxhash.h): XXHASH_USE_VENDOR = 0
equals(XXHASH_USE_VENDOR, 1) {
    INCLUDEPATH += $$SPEC/../../../lib/xxhash
    HEADERS += $$SPEC/../../../lib/xxhash/xxhash.h
    SOURCES += $$SPEC/../../../lib/xxhash/xxhash.c
} else {
    LIBS += -lxxhash
}
