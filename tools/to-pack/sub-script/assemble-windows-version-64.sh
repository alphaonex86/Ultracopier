#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

cd ${TEMP_PATH}/

COMPIL_DEBUGREAL=1
COMPIL_NORMAL=1
COMPIL_PLUGIN=1
COMPIL_ULTIMATE=1

ARCHITECTURE="x86_64"

source ${BASE_PWD}/sub-script/assemble.sh

if [ ${COMPIL_DEBUGREAL} -eq 1 ]
then
        assemble "ultracopier-debug-real" "${ARCHITECTURE}" 1 1 0 0 0
fi
if [ ${COMPIL_NORMAL} -eq 1 ]
then
	assemble "ultracopier-debug" "${ARCHITECTURE}" 1 0 0 0 0
	assemble "ultracopier-debug-portable" "${ARCHITECTURE}" 1 0 1 0 0
	assemble "ultracopier-portable" "${ARCHITECTURE}" 0 0 1 0 0
	assemble "ultracopier" "${ARCHITECTURE}" 0 0 0 0 0

	assemble "ultracopier-debug-static" "${ARCHITECTURE}" 1 0 0 0 1
	assemble "ultracopier-debug-portable-static" "${ARCHITECTURE}" 1 0 1 0 1
	assemble "ultracopier-portable-static" "${ARCHITECTURE}" 0 0 1 0 1
	assemble "ultracopier-static" "${ARCHITECTURE}" 0 0 0 0 1
fi

if [ ${COMPIL_ULTIMATE} -eq 1 ]
then
	assemble "ultracopier-ultimate" "${ARCHITECTURE}" 0 0 0 1 0
	assemble "ultracopier-ultimate-sse2" "${ARCHITECTURE}" 0 0 0 1 0
	assemble "ultracopier-ultimate-sse3" "${ARCHITECTURE}" 0 0 0 1 0
	assemble "ultracopier-ultimate-core2" "${ARCHITECTURE}" 0 0 0 1 0
	assemble "ultracopier-ultimate-core-i" "${ARCHITECTURE}" 0 0 0 1 0

	assemble "ultracopier-ultimate-k8" "${ARCHITECTURE}" 0 0 0 1 0
	assemble "ultracopier-ultimate-barcelona" "${ARCHITECTURE}" 0 0 0 1 0
	assemble "ultracopier-ultimate-bobcat" "${ARCHITECTURE}" 0 0 0 1 0
	assemble "ultracopier-ultimate-llano" "${ARCHITECTURE}" 0 0 0 1 0
	assemble "ultracopier-ultimate-bulldozer" "${ARCHITECTURE}" 0 0 0 1 0
fi

if [ ${COMPIL_PLUGIN} -eq 1 ]
then

TARGET="ultracopier"
find ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>windows-x86_64<\/architecture>/g" {} \; > /dev/null 2>&1
cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins/
for plugins_cat in `ls -1`
do
	if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
	then
		cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins/${plugins_cat}/
		for plugins_name in `ls -1`
		do
			if [ -f ${plugins_name}/informations.xml ]
			then
				ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
				if [ -d ${plugins_name} ] && [ ! -f ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}.urc ] && [ -f ${plugins_name}/*.dll ]
				then
					echo "pack the ${ARCHITECTURE} windows for the plugin: ${plugins_cat}/${plugins_name}"
					mkdir -p ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/
					tar --posix -c -f - ${plugins_name}/ | xz -9 --check=crc32 > ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}.urc
				fi
			fi
		done
	fi
	cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins/
done
cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins-alternative/
for plugins_cat in `ls -1`
do
	if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
	then
		cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins-alternative/${plugins_cat}/
		for plugins_name in `ls -1`
		do
			if [ -f ${plugins_name}/informations.xml ]
			then
				ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
				if [ -d ${plugins_name} ] && [ ! -f ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}.urc ] && [ -f ${plugins_name}/*.dll ]
				then
					echo "pack the ${ARCHITECTURE} windows for the alternative plugin: ${plugins_cat}/${plugins_name}"
					mkdir -p ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/
					tar --posix -c -f - ${plugins_name}/ | xz -9 --check=crc32 > ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}.urc
				fi
			fi
		done
	fi
	cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins-alternative/
done


TARGET="ultracopier-debug"
find ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>windows-x86_64<\/architecture>/g" {} \; > /dev/null 2>&1
cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins/
for plugins_cat in `ls -1`
do
	if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
	then
		cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins/${plugins_cat}/
		for plugins_name in `ls -1`
		do
			if [ -f ${plugins_name}/informations.xml ]
			then
				ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
				if [ -d ${plugins_name} ] && [ ! -f ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}-debug.urc ] && [ -f ${plugins_name}/*.dll ]
				then
					echo "pack the ${ARCHITECTURE} debug windows for the plugin: ${plugins_cat}/${plugins_name}"
					mkdir -p ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/
					tar --posix -c -f - ${plugins_name}/ | xz -9 --check=crc32 > ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}-debug.urc
				fi
			fi
		done
	fi
	cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins/
done
cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins-alternative/
for plugins_cat in `ls -1`
do
	if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
	then
		cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins-alternative/${plugins_cat}/
		for plugins_name in `ls -1`
		do
			if [ -f ${plugins_name}/informations.xml ]
			then
				ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
				if [ -d ${plugins_name} ] && [ ! -f ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}-debug.urc ] && [ -f ${plugins_name}/*.dll ]
				then
					echo "pack the ${ARCHITECTURE} debug windows for the alternative plugin: ${plugins_cat}/${plugins_name}"
					mkdir -p ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/
					tar --posix -c -f - ${plugins_name}/ | xz -9 --check=crc32 > ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-windows-${ARCHITECTURE}-debug.urc
				fi
			fi
		done
	fi
	cd ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}-for-plugins/plugins-alternative/
done

fi
