#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
        exit;
fi

cd ${TEMP_PATH}/

source ${BASE_PWD}/sub-script/compil.sh
source ${BASE_PWD}/sub-script/assemble.sh

ARCHITECTURE="x86"
#TARGETTEMPTEST="ultracopier-ultimate-debug-real"
#compil "${TARGETTEMPTEST}-windows-${ARCHITECTURE}" 1 1 0 0 32 "-g -mtune=generic -march=i686" 0 0 0 0 0 0
#assemble "${TARGETTEMPTEST}" "${ARCHITECTURE}" 1 1 0 0 0 0 0
#echo http://ultracopier.first-world.info/temp/${TARGETTEMPTEST}-windows-${ARCHITECTURE}-${ULTRACOPIER_VERSION}.7z

TARGETTEMPTEST="ultracopier-ultimate-debug"
compil "${TARGETTEMPTEST}-windows-${ARCHITECTURE}" 1 0 0 0 32 "-g -mtune=generic -march=i686" 0 0 0 0 0 0
assemble "${TARGETTEMPTEST}" "${ARCHITECTURE}" 1 0 0 0 0 0 0
echo http://ultracopier.first-world.info/temp/${TARGETTEMPTEST}-windows-${ARCHITECTURE}-${ULTRACOPIER_VERSION}.zip

ARCHITECTURE="x86_64"

#TARGETTEMPTEST="ultracopier-ultimate-cgminer-debug"
#compil "ultracopier-ultimate-cgminer-debug-windows-${ARCHITECTURE}" 1 0 0 0 64 "-mtune=generic -march=nocona" 1 0 0 1 0 0
#assemble "${TARGETTEMPTEST}" "${ARCHITECTURE}" 1 0 0 1 0 1 0
#echo http://ultracopier.first-world.info/temp/${TARGETTEMPTEST}-windows-${ARCHITECTURE}-${ULTRACOPIER_VERSION}.zip

#TARGETTEMPTEST="ultracopier-ultimate-cgminer"
#compil "ultracopier-ultimate-cgminer-windows-${ARCHITECTURE}" 0 0 0 0 64 "-mtune=generic -march=nocona" 1 0 0 1 0 0
#assemble "${TARGETTEMPTEST}" "${ARCHITECTURE}" 0 0 0 1 0 1 0
#echo http://ultracopier.first-world.info/temp/${TARGETTEMPTEST}-windows-${ARCHITECTURE}-${ULTRACOPIER_VERSION}.zip