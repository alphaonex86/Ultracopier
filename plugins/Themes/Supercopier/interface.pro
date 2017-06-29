DEFINES+=SUPERCOPIER

include(../Oxygen/interface.pro)

!CONFIG(static) {
RESOURCES	-= \
    ../Oxygen/interfaceResources.qrc \
    ../Oxygen/interfaceResources_unix.qrc \
    ../Oxygen/interfaceResources_windows.qrc

RESOURCES	+= \
    ../Supercopier/interfaceResources.qrc \
    ../Supercopier/interfaceResources_unix.qrc \
    ../Supercopier/interfaceResources_windows.qrc
}
