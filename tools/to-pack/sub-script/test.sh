#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
        exit;
fi

cd ${TEMP_PATH}/

source ${BASE_PWD}/sub-script/compil.sh
compil "ultracopier-debug-cgminer-portable-windows-x86_64" 1 0 1 0 64 "-mtune=generic -march=nocona" 0 0 0 1

ARCHITECTURE="x86_64"

source ${BASE_PWD}/sub-script/assemble.sh

assemble "ultracopier-debug-cgminer-portable" "${ARCHITECTURE}" 1 0 1 0 0 1

