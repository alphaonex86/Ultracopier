QT += widgets
DEFINES += UNICODE _UNICODE
TEMPLATE        = lib
CONFIG         += plugin
HEADERS         = \
                StructEnumDefinition.h \
    StructEnumDefinition_CopyEngine.h \
    DebugEngineMacro.h \
    Variable.h \
    TransferThread.h \
    ReadThread.h \
    WriteThread.h \
    MkPath.h \
    AvancedQFile.h \
    ListThread.h \
    ../../../interface/PluginInterface_CopyEngine.h \
    ../../../interface/OptionInterface.h \
    ../../../interface/FacilityInterface.h \
    Filters.h \
    FilterRules.h \
    RenamingRules.h \
    DriveManagement.h \
    CopyEngine.h \
    DebugDialog.h \
    Factory.h \
    FileErrorDialog.h \
    FileExistsDialog.h \
    FileIsSameDialog.h \
    FolderExistsDialog.h \
    ScanFileOrFolder.h \
    DiskSpace.h
SOURCES         = \
    TransferThread.cpp \
    ReadThread.cpp \
    WriteThread.cpp \
    MkPath.cpp \
    AvancedQFile.cpp \
    ListThread.cpp \
    Filters.cpp \
    FilterRules.cpp \
    RenamingRules.cpp \
    ListThread_InodeAction.cpp \
    DriveManagement.cpp \
    CopyEngine-collision-and-error.cpp \
    CopyEngine.cpp \
    DebugDialog.cpp \
    Factory.cpp \
    FileErrorDialog.cpp \
    FileExistsDialog.cpp \
    FileIsSameDialog.cpp \
    FolderExistsDialog.cpp \
    ScanFileOrFolder.cpp \
    DiskSpace.cpp
TARGET          = $$qtLibraryTarget(copyEngine)
include(../../../updateqm.pri)
TRANSLATIONS += Languages/ar/translation.ts \
    Languages/de/translation.ts \
    Languages/el/translation.ts \
    Languages/en/translation.ts \
    Languages/es/translation.ts \
    Languages/fr/translation.ts \
    Languages/hi/translation.ts \
    Languages/hu/translation.ts \
    Languages/id/translation.ts \
    Languages/it/translation.ts \
    Languages/ja/translation.ts \
    Languages/ko/translation.ts \
    Languages/nl/translation.ts \
    Languages/no/translation.ts \
    Languages/pl/translation.ts \
    Languages/pt/translation.ts \
    Languages/ru/translation.ts \
    Languages/th/translation.ts \
    Languages/tr/translation.ts \
    Languages/zh/translation.ts

include(../../../extratool.pri)
target.path     = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
translations.files = Languages
translations.path = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
infos.files      = informations.xml
infos.path       = $${PREFIX}/lib/ultracopier/$$superBaseName(_PRO_FILE_PWD_)
INSTALLS       += target translations infos

FORMS += \
    fileErrorDialog.ui \
    fileExistsDialog.ui \
    fileIsSameDialog.ui \
    debugDialog.ui \
    folderExistsDialog.ui \
    Filters.ui \
    FilterRules.ui \
    RenamingRules.ui \
    copyEngineOptions.ui \
    DiskSpace.ui

OTHER_FILES += informations.xml

!CONFIG(static) {
RESOURCES += \
    copyEngineResources.qrc
}


win32: {
    win32-g++*: {
        LIBS += -luser32 -lgdi32 -lpowrprof -lbthprops -lws2_32 -lmsvfw32 -lavicap32 -luuid
    }
    HEADERS += qstorageinfo_win_p.h \
    windows/qwmihelper_win_p.h

    SOURCES += qstorageinfo_win.cpp \
    windows/qwmihelper_win.cpp

       LIBS += \
            -lOle32 \
            -lUser32 \
            -lGdi32 \
            -lIphlpapi \
            -lOleaut32 \
            -lPowrProf \
            -lSetupapi

  win32-g++: {
        LIBS += -luser32 -lgdi32
    }

}

linux-*: !simulator: {
    HEADERS += qstorageinfo_linux_p.h

    SOURCES += qstorageinfo_linux.cpp

    qtHaveModule(dbus) {
        config_ofono: {
            QT += dbus
            HEADERS += qofonowrapper_p.h
            SOURCES += qofonowrapper.cpp
        } else {
            DEFINES += QT_NO_OFONO
        }

        config_udisks {
            QT_PRIVATE += dbus
        } else: {
            DEFINES += QT_NO_UDISKS
        }
    } else {
        DEFINES += QT_NO_OFONO QT_NO_UDISKS
    }

    config_udev {
        CONFIG += link_pkgconfig
        PKGCONFIG += udev
        LIBS += -ludev
        HEADERS += qudevwrapper_p.h
        SOURCES += qudevwrapper.cpp
    } else {
        DEFINES += QT_NO_UDEV
    }

    config_libsysinfo {
        CONFIG += link_pkgconfig
        PKGCONFIG += sysinfo
        LIBS += -lsysinfo
    } else: {
        DEFINES += QT_NO_LIBSYSINFO
    }
}

macx: {
         OBJECTIVE_SOURCES += qstorageinfo_mac.mm

         HEADERS += qstorageinfo_mac_p.h

         LIBS += -framework SystemConfiguration \
                -framework Foundation \
                -framework IOKit  \
                -framework QTKit \
                -framework CoreWLAN \
                -framework CoreLocation \
                -framework CoreFoundation \
                -framework ScreenSaver \
                -framework IOBluetooth \
                -framework CoreServices \
                -framework DiskArbitration \
                -framework ApplicationServices
}
