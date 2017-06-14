DEFINES+=SUPERCOPIER

include(../Themes/Oxygen/interface.pro)

!CONFIG(static) {
RESOURCES	+= \
    interfaceResources.qrc \
    interfaceResources_unix.qrc \
    interfaceResources_windows.qrc
}
