TEMPLATE        = lib
CONFIG		+= plugin
QT		+= network
win32:LIBS += -ladvapi32
HEADERS         = listener.h \
    catchcopy-api-0002/VariablesCatchcopy.h \
    catchcopy-api-0002/ServerCatchcopy.h \
    catchcopy-api-0002/ExtraSocketCatchcopy.h \
    Environment.h \
    Variable.h \
    DebugEngineMacro.h \
    StructEnumDefinition.h \
    ../../../interface/PluginInterface_Listener.h
SOURCES         = listener.cpp \
    catchcopy-api-0002/ServerCatchcopy.cpp \
    catchcopy-api-0002/ExtraSocketCatchcopy.cpp
TARGET          = $$qtLibraryTarget(listener)
include(../../../extratool.pri)
target.path     = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
infos.files      = informations.xml
infos.path       = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
INSTALLS       += target infos

