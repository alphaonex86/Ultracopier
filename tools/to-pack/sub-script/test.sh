#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
        exit;
fi

cd ${TEMP_PATH}/

source ${BASE_PWD}/sub-script/compil.sh
compil "ultracopier-debug-portable-windows-x86" 1 0 1 0 32 "-mtune=generic -march=i686" 0 0 0 0 0

ARCHITECTURE="x86"

source ${BASE_PWD}/sub-script/assemble.sh

assemble "ultracopier-debug-portable" "${ARCHITECTURE}" 1 0 1 0 0 0 0

