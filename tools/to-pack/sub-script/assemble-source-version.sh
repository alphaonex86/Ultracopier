#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

cd ${TEMP_PATH}/

FINAL_ARCHIVE="ultracopier-src-${ULTRACOPIER_VERSION}.tar.xz"
if [ ! -e ${FINAL_ARCHIVE} ]; then
	rm -Rf ${TEMP_PATH}/ultracopier-src/
	cp -aRf ${ULTRACOPIERSOURCESPATH}/ ${TEMP_PATH}/ultracopier-src/
	find ${TEMP_PATH}/ultracopier-src/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/ultracopier-src/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/ultracopier-src/ -name "*-build-desktop*" -type d -exec rm -Rf {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/ultracopier-src/ -name "*Qt_in_*" -type d -exec rm -Rf {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/ultracopier-src/ -name "informations.xml" -exec sed -i "s/linux-x86_64-pc/windows-x86/g" {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/ultracopier-src/ -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${ULTRACOPIER_VERSION}<\/version>/g" {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/ultracopier-src/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_DEBUG/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/ultracopier-src/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_PORTABLE/\/\/#define ULTRACOPIER_VERSION_PORTABLE/g" {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/ultracopier-src/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/ultracopier-src/ -iname "*.qm" -exec rm {} \; > /dev/null 2>&1

	tar cJf ${FINAL_ARCHIVE} ultracopier-src/ --owner=0 --group=0 --mtime='2010-01-01' -H ustar
	if [ ! -e ${FINAL_ARCHIVE} ]; then
		echo "${FINAL_ARCHIVE} not exists!";
		exit;
	fi
fi

if [ -d ${TEMP_PATH}/ultracopier-src/plugins/ ]
then
	cd ${TEMP_PATH}/ultracopier-src/plugins/
	find ${TEMP_PATH}/ultracopier-src/plugins/ -iname "*.qm" -exec rm {} \;
	for plugins_cat in `ls -1`
	do
		if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
		then
			cd ${TEMP_PATH}/ultracopier-src/plugins/${plugins_cat}/
			for plugins_name in `ls -1`
			do
				if [ -d ${plugins_name} ] &&  [ -f ${plugins_name}/informations.xml ]
				then
					find ${plugins_name}/ -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${ULTRACOPIER_VERSION}<\/version>/g" {} \; > /dev/null 2>&1
					echo "do source package for the plugin: ${plugins_cat}/${plugins_name}"
					mkdir -p ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/
					ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
					find ${TEMP_PATH}/ultracopier-src/ -iname "*.h" -exec sed -i "s/..\/..\/..\/interface\//interface\//g" {} \; > /dev/null 2>&1
					find ${TEMP_PATH}/ultracopier-src/ -iname "*.pro" -exec sed -i "s/..\/..\/..\/interface\//interface\//g" {} \; > /dev/null 2>&1
					rsync -art ${TEMP_PATH}/ultracopier-src/interface/ ${plugins_name}/interface/ > /dev/null 2>&1
					FILE="StructEnumDefinition.h"
					if [ -e ${plugins_name}/${FILE} ]
					then
						rm -f ${plugins_name}/${FILE}
						cp ${TEMP_PATH}/ultracopier-src/${FILE} ${plugins_name}/${FILE}
					fi
					FILE="CompilerInfo.h"
					if [ -e ${plugins_name}/${FILE} ]
					then
						rm -f ${plugins_name}/${FILE}
						cp ${TEMP_PATH}/ultracopier-src/${FILE} ${plugins_name}/${FILE}
					fi
					FILE="PlatformMacro.h"
					if [ -e ${plugins_name}/${FILE} ]
					then
						rm -f ${plugins_name}/${FILE}
						cp ${TEMP_PATH}/ultracopier-src/${FILE} ${plugins_name}/${FILE}
					fi
					tar -c -f - ${plugins_name}/ --owner=0 --group=0 --mtime='2010-01-01' -H ustar | xz -9 > ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-src.tar.xz
				fi
			done
			cd ${TEMP_PATH}/ultracopier-src/plugins/
		fi
	done
	cd ${TEMP_PATH}/ultracopier-src/plugins-alternative/
	find ${TEMP_PATH}/ultracopier-src/plugins-alternative/ -iname "*.qm" -exec rm {} \;
	for plugins_cat in `ls -1`
	do
		if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
		then
			cd ${TEMP_PATH}/ultracopier-src/plugins-alternative/${plugins_cat}/
			for plugins_name in `ls -1`
			do
				if [ -d ${plugins_name} ] &&  [ -f ${plugins_name}/informations.xml ]
				then
					find ${plugins_name}/ -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${ULTRACOPIER_VERSION}<\/version>/g" {} \; > /dev/null 2>&1
					echo "do source package for the plugin: ${plugins_cat}/${plugins_name}"
					mkdir -p ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/
					ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
					find ${TEMP_PATH}/ultracopier-src/ -iname "*.h" -exec sed -i "s/..\/..\/..\/interface\//interface\//g" {} \; > /dev/null 2>&1
					find ${TEMP_PATH}/ultracopier-src/ -iname "*.pro" -exec sed -i "s/..\/..\/..\/interface\//interface\//g" {} \; > /dev/null 2>&1
					rsync -art ${TEMP_PATH}/ultracopier-src/interface/ ${plugins_name}/interface/ > /dev/null 2>&1
					FILE="StructEnumDefinition.h"
					if [ -e ${plugins_name}/${FILE} ]
					then
						rm -f ${plugins_name}/${FILE}
						cp ${TEMP_PATH}/ultracopier-src/${FILE} ${plugins_name}/${FILE}
					fi
					FILE="CompilerInfo.h"
					if [ -e ${plugins_name}/${FILE} ]
					then
						rm -f ${plugins_name}/${FILE}
						cp ${TEMP_PATH}/ultracopier-src/${FILE} ${plugins_name}/${FILE}
					fi
					FILE="PlatformMacro.h"
					if [ -e ${plugins_name}/${FILE} ]
					then
						rm -f ${plugins_name}/${FILE}
						cp ${TEMP_PATH}/ultracopier-src/${FILE} ${plugins_name}/${FILE}
					fi
					tar -c -f - ${plugins_name}/ --owner=0 --group=0 --mtime='2010-01-01' -H ustar | xz -9 > ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-src.tar.xz
				fi
			done
			cd ${TEMP_PATH}/ultracopier-src/plugins-alternative/
		fi
	done
	cd ${TEMP_PATH}/ultracopier-src/plugins/Languages/
	for plugins_name in `ls -1`
	do
		if [ -d ${plugins_name} ]
		then
			mkdir -p ${TEMP_PATH}/plugins/Languages/${plugins_name}/
			find ${plugins_name}/ -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${ULTRACOPIER_VERSION}<\/version>/g" {} \; > /dev/null 2>&1
			ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
			tar -c -f - ${plugins_name}/ --owner=0 --group=0 --mtime='2010-01-01' -H ustar | xz -9 > ${TEMP_PATH}/plugins/Languages/${plugins_name}/Languages-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-src.tar.xz
		fi
	done
fi
