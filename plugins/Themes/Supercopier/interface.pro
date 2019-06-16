DEFINES+=SUPERCOPIER

include($$PWD/../Oxygen/interfaceInclude.pri)

!CONFIG(static) {
RESOURCES	+= \
    $$PWD/../Supercopier/interfaceResources.qrc \
    $$PWD/../Supercopier/interfaceResources_unix.qrc \
    $$PWD/../Supercopier/interfaceResources_windows.qrc
}
