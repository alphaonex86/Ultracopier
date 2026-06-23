# Standalone unit test for uc_foldCompletedWrite() (pipeline/ContiguousWatermark.h) -- the
# contiguous-from-0 low-water mark used by the pipelined backends' media-reconnect resume.
# ContiguousWatermark.h is dependency-free (only <vector>/<utility>/<cstdint>), so NO Qt.
CONFIG   += console c++17
CONFIG   -= app_bundle qt
QT        =
TEMPLATE  = app
TARGET    = watermark_test
SOURCES  += $$PWD/watermark_test.cpp
HEADERS  += $$PWD/../../pipeline/ContiguousWatermark.h
