TEMPLATE        = lib
CONFIG         += plugin
HEADERS         = copyEngine.h \
		PluginInterface_CopyEngine.h \
		StructEnumDefinition.h \
    scanFileOrFolder.h \
    writeThread.h \
    readThread.h \
    fileErrorDialog.h \
    fileExistsDialog.h \
    fileIsSameDialog.h \
    factory.h \
    interface/PluginInterface_CopyEngine.h \
    StructEnumDefinition_CopyEngine.h \
    DebugEngineMacro.h \
    Variable.h \
    debugDialog.h \
    folderExistsDialog.h \
    AvancedQFile.h \
    ../../../interface/PluginInterface_CopyEngine.h
SOURCES         = copyEngine.cpp \
    scanFileOrFolder.cpp \
    writeThread.cpp \
    readThread.cpp \
    fileErrorDialog.cpp \
    fileExistsDialog.cpp \
    fileIsSameDialog.cpp \
    factory.cpp \
    debugDialog.cpp \
    folderExistsDialog.cpp \
    AvancedQFile.cpp
TARGET          = $$qtLibraryTarget(copyEngine)

FORMS += \
    options.ui \
    fileErrorDialog.ui \
    fileExistsDialog.ui \
    fileIsSameDialog.ui \
    debugDialog.ui \
    folderExistsDialog.ui
