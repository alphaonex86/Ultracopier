#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
        exit;
fi

cd ${TEMP_PATH}/

source ${BASE_PWD}/sub-script/compil.sh
source ${BASE_PWD}/sub-script/assemble.sh

ARCHITECTURE="x86"
#compil "supercopier-windows-x86" 0 0 0 0 32 "-mtune=generic -march=i686" 0 0 0 0 1 0
#assemble "supercopier" "${ARCHITECTURE}" 0 0 0 0 0 0 1

        compil "ultracopier-debug-windows-x86" 1 0 0 0 32 "-mtune=generic -march=i686" 0 0 0 0 0 0
        assemble "ultracopier-debug" "${ARCHITECTURE}" 1 0 0 0 0 0 0
        #compil "ultracopier-debug-portable-windows-x86" 1 0 1 0 32 "-mtune=generic -march=i686" 0 0 0 0 0 0
        #assemble "ultracopier-debug-portable" "${ARCHITECTURE}" 1 0 1 0 0 0 0
        compil "ultracopier-portable-windows-x86" 0 0 1 0 32 "-mtune=generic -march=i686" 0 0 0 0 0 0
        assemble "ultracopier-portable" "${ARCHITECTURE}" 0 0 1 0 0 0 0
        #compil "ultracopier-portableapps-windows-x86" 0 0 1 1 32 "-mtune=generic -march=i686" 0 0 0 0 0 0
        #assemble "ultracopier-portableapps" "${ARCHITECTURE}" 0 0 1 0 0 0 0
        compil "ultracopier-windows-x86" 0 0 0 0 32 "-mtune=generic -march=i686" 0 0 0 0 0 0
        assemble "ultracopier" "${ARCHITECTURE}" 0 0 0 0 0 0 0
        #compil "ultracopier-debug-static-windows-x86" 1 0 0 0 32 "-mtune=generic -march=i686" 0 0 1 0 0 0
        #assemble "ultracopier-debug-static" "${ARCHITECTURE}" 1 0 0 0 1 0 0
        #compil "ultracopier-debug-portable-static-windows-x86" 1 0 1 0 32 "-mtune=generic -march=i686" 0 0 1 0 0 0
        #assemble "ultracopier-debug-portable-static" "${ARCHITECTURE}" 1 0 1 0 1 0 0
        #compil "ultracopier-portable-static-windows-x86" 0 0 1 0 32 "-mtune=generic -march=i686" 0 0 1 0 0 0
        #assemble "ultracopier-portable-static" "${ARCHITECTURE}" 0 0 1 0 1 0 0
        #compil "ultracopier-static-windows-x86" 0 0 0 0 32 "-mtune=generic -march=i686" 0 0 1 0 0 0
        #assemble "ultracopier-static" "${ARCHITECTURE}" 0 0 0 0 1 0 0
        compil "supercopier-ultimate-windows-x86" 0 0 0 0 32 "-mtune=generic -march=i686" 1 0 0 0 1 0
        assemble "supercopier-ultimate" "${ARCHITECTURE}" 0 0 0 1 0 0 1
        compil "supercopier-ultimate-cgminer-windows-x86" 0 0 0 0 32 "-mtune=generic -march=i686" 1 0 0 1 1 0
        assemble "supercopier-ultimate-cgminer" "${ARCHITECTURE}" 0 0 0 1 0 1 1
        #compil "supercopier-ultimate-cgminer-static-windows-x86" 0 0 0 0 32 "-mtune=generic -march=i686" 1 0 1 1 1 0
        #assemble "supercopier-ultimate-cgminer-static" "${ARCHITECTURE}" 0 0 0 1 1 1 1
