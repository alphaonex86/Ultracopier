DEFINES+=SUPERCOPIER

include(../Oxygen/interface.pro)

!CONFIG(static) {
RESOURCES	+= \
    interfaceResources.qrc \
    interfaceResources_unix.qrc \
    interfaceResources_windows.qrc

RESOURCES	-= \
    ../Oxygen/interfaceResources.qrc \
    ../Oxygen/interfaceResources_unix.qrc \
    ../Oxygen/interfaceResources_windows.qrc
}
