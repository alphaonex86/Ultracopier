include($$PWD/../Oxygen2/interfaceInclude.pri)

!CONFIG(static) {
RESOURCES	+= \
    $$PWD/../Oxygen2/interfaceResources.qrc \
    $$PWD/../Oxygen2/interfaceResources_unix.qrc \
    $$PWD/../Oxygen2/interfaceResources_windows.qrc
}
