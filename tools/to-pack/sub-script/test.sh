#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
        exit;
fi

cd ${TEMP_PATH}/

source ${BASE_PWD}/sub-script/compil.sh
ARCHITECTURE="x86"
source ${BASE_PWD}/sub-script/assemble.sh

TARGETTEMPTEST="supercopier-ultimate-debug"
compil "${TARGETTEMPTEST}-windows-x86" 1 0 0 0 32 "-mtune=generic -march=i686" 1 0 0 0 1
assemble "${TARGETTEMPTEST}" "${ARCHITECTURE}" 1 0 0 1 0 0 1
echo http://ultracopier.first-world.info/temp/${TARGETTEMPTEST}-windows-x86-1.2.0.2.zip

