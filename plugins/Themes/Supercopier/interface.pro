DEFINES+=SUPERCOPIER

include($$PWD/../Oxygen/interfaceInclude.pri)
TEMPLATE        = lib
CONFIG         += plugin

!CONFIG(static) {
RESOURCES	+= \
    $$PWD/../Supercopier/interfaceResources.qrc \
    $$PWD/../Supercopier/interfaceResources_unix.qrc \
    $$PWD/../Supercopier/interfaceResources_windows.qrc
}
