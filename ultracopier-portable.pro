include(ultracopier.pro)

TARGET = ultracopier-portable

DEFINES = ULTRACOPIER_VERSION_PORTABLE

RESOURCES +=

if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
    mac:LIBS = $$member(LIBS, 0) $$member(LIBS, 1)_debug
    win32:LIBS = $$member(LIBS, 0) $$member(LIBS, 1)d
 }
