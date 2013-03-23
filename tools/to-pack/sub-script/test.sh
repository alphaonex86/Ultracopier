#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
        exit;
fi

cd ${TEMP_PATH}/

source ${BASE_PWD}/sub-script/compil.sh
#compil "ultracopier-debug-portable-static-windows-x86" 1 0 1 0 32 "-mtune=generic -march=i686" 0 0 1
compil "ultracopier-portable-static-windows-x86" 0 0 1 0 32 "-mtune=generic -march=i686" 0 0 1

ARCHITECTURE="x86"

source ${BASE_PWD}/sub-script/assemble.sh

#assemble "ultracopier-debug-portable-static" "${ARCHITECTURE}" 1 0 1 0 1
assemble "ultracopier-portable-static" "${ARCHITECTURE}" 0 0 1 0 1

