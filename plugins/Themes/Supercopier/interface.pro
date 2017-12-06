DEFINES+=SUPERCOPIER

include($$PWD/../Oxygen/interface.pro)

!CONFIG(static) {
RESOURCES	-= \
    $$PWD/../Oxygen/interfaceResources.qrc \
    $$PWD/../Oxygen/interfaceResources_unix.qrc \
    $$PWD/../Oxygen/interfaceResources_windows.qrc

RESOURCES	+= \
    $$PWD/../Supercopier/interfaceResources.qrc \
    $$PWD/../Supercopier/interfaceResources_unix.qrc \
    $$PWD/../Supercopier/interfaceResources_windows.qrc
}
