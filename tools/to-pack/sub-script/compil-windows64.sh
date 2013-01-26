#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

COMPIL_DEBUGREAL=1
COMPIL_NORMAL=1
COMPIL_PLUGIN=1
COMPIL_ULTIMATE=1

cd ${TEMP_PATH}/

source ${BASE_PWD}/sub-script/compil.sh

if [ ${COMPIL_DEBUGREAL} -eq 1 ]
then
        compil "ultracopier-debug-real-windows-x86_64" 1 1 0 0 64 "" 0 0
fi
if [ ${COMPIL_NORMAL} -eq 1 ]
then
	compil "ultracopier-debug-windows-x86_64" 1 0 0 0 64 "" 0 0
	compil "ultracopier-debug-portable-windows-x86_64" 1 0 1 0 64 "" 0 0
	compil "ultracopier-portable-windows-x86_64" 0 0 1 0 64 "" 0 0
	compil "ultracopier-windows-x86_64" 0 0 0 0 64 "" 0 0
fi

if [ ${COMPIL_PLUGIN} -eq 1 ]
then
	compil "ultracopier-windows-x86_64-for-plugins" 0 0 0 0 64 "" 0 1
	compil "ultracopier-debug-windows-x86_64-for-plugins" 1 0 0 0 64 "" 0 1
fi

if [ ${COMPIL_ULTIMATE} -eq 1 ]
then
	compil "ultracopier-ultimate-windows-x86_64" 0 0 0 0 64 "-msse -msse2" 1 0
	compil "ultracopier-ultimate-sse2-windows-x86_64" 0 0 0 0 64 "-msse -msse2" 1 0
	compil "ultracopier-ultimate-sse3-windows-x86_64" 0 0 0 0 64 "-msse -msse2 -msse3" 1 0
	compil "ultracopier-ultimate-core2-windows-x86_64" 0 0 0 0 64 "-msse -msse2 -msse3 -march=core2" 1 0
	compil "ultracopier-ultimate-core-i-windows-x86_64" 0 0 0 0 64 "-msse -msse2 -msse3 -march=core2" 1 0

	compil "ultracopier-ultimate-k8-windows-x86_64" 0 0 0 0 64 "-msse -msse2 -march=k8" 1 0
	compil "ultracopier-ultimate-barcelona-windows-x86_64" 0 0 0 0 64 "-msse -msse2 -march=k8" 1 0
	compil "ultracopier-ultimate-bobcat-windows-x86_64" 0 0 0 0 64 "-march=amdfam10 -mno-3dnow -mcx16 -mpopcnt -mssse3 -mmmx" 1 0
	compil "ultracopier-ultimate-llano-windows-x86_64" 0 0 0 0 64 "-march=amdfam10 -mcx16 -mpopcnt -pipe" 1 0
	compil "ultracopier-ultimate-bulldozer-windows-x86_64" 0 0 0 0 64 "-O2 -pipe -fomit-frame-pointer -march=amdfam10 -mcx16 -msahf -maes -mpclmul -mpopcnt -mabm" 1 0
fi
