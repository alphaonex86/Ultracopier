# Standalone unit test for the operation-decomposition scheduler CORE (pipeline/OpScheduler.h) --
# STAGE 1a of the tiered-parallelism pipelining. OpScheduler.h is dependency-free (only
# <vector>/<cstdint>), so NO Qt.
CONFIG   += console c++17
CONFIG   -= app_bundle qt
QT        =
TEMPLATE  = app
TARGET    = opscheduler_test
SOURCES  += $$PWD/opscheduler_test.cpp
HEADERS  += $$PWD/../../pipeline/OpScheduler.h
