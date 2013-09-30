TEMPLATE        = lib
CONFIG         += plugin
LIBS += -ladvapi32
HEADERS         = sessionLoader.h \
    StructEnumDefinition.h \
    Variable.h \
    Environment.h \
    DebugEngineMacro.h \
    ../../../interface/PluginInterface_SessionLoader.h
SOURCES         = sessionLoader.cpp
TARGET          = $$qtLibraryTarget(sessionLoader)
include(../../../extratool.pri)
target.path     = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
infos.files      = informations.xml
infos.path       = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
INSTALLS       += target infos

