include($$PWD/../Oxygen/interfaceInclude.pri)
TEMPLATE        = lib
CONFIG         += plugin

!CONFIG(static) {
RESOURCES	+= \
    $$PWD/../Oxygen/interfaceResources.qrc \
    $$PWD/../Oxygen/interfaceResources_unix.qrc \
    $$PWD/../Oxygen/interfaceResources_windows.qrc
}
