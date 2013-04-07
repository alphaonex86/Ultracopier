#!/bin/bash

function compil {
	TARGET=$1
	DEBUG=$2
	DEBUG_REAL=$3
	PORTABLE=$4
	PORTABLEAPPS=$5
	BITS=$6
	CFLAGSCUSTOM="$7"
	ULTIMATE=$8
	FORPLUGIN=$9
	STATIC=${10}
	cd ${BASE_PWD}
	echo "${TARGET} rsync..."
	if [ $FORPLUGIN -eq 1 ]
	then
		rsync -aqrt ${ULTRACOPIERSOURCESPATH} ${TEMP_PATH}/${TARGET}/
	else
		if [ $ULTIMATE -eq 1 ]
		then
			rsync -aqrt ${ULTRACOPIERSOURCESPATH} ${TEMP_PATH}/${TARGET}/ --exclude=/plugins-alternative/CopyEngine/Rsync/
		else
			rsync -aqrt ${ULTRACOPIERSOURCESPATH} ${TEMP_PATH}/${TARGET}/ --exclude=/plugins-alternative/
		fi
	fi
	for project in `find ${TEMP_PATH}/${TARGET}/plugins/Languages/ -mindepth 1 -type d`
	do
		cd ${project}/
		if [ -f *.ts ]
		then
			lrelease -nounfinished -compress -removeidentical *.ts > /dev/null 2>&1
		fi
		cd ${TEMP_PATH}/${TARGET}/
	done
	find ${TEMP_PATH}/${TARGET}/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/${TARGET}/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
	find ${TEMP_PATH}/${TARGET}/ -name "informations.xml" -exec sed -i -r "s/ultracopier-1\.[0-9]+\.[0-9]+\.[0-9]+/ultracopier-${ULTRACOPIER_VERSION}/g" {} \; > /dev/null 2>&1
	if [ $DEBUG -eq 1 ]
	then
		find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_DEBUG/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
	else
		find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_DEBUG/\/\/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
	fi
        if [ $STATIC -eq 1 ]
        then
                find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/g" {} \; > /dev/null 2>&1
        else

                find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/\/\/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/g" {} \; > /dev/null 2>&1
        fi
	if [ $ULTIMATE -eq 1 ]
	then
		find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_VERSION_ULTIMATE/#define ULTRACOPIER_VERSION_ULTIMATE/g" {} \; > /dev/null 2>&1
	else
		find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_ULTIMATE/\/\/#define ULTRACOPIER_VERSION_ULTIMATE/g" {} \; > /dev/null 2>&1
	fi
	if [ $PORTABLE -eq 1 ]
	then
		find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_VERSION_PORTABLE/#define ULTRACOPIER_VERSION_PORTABLE/g" {} \; > /dev/null 2>&1
	else
		find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_PORTABLE/\/\/#define ULTRACOPIER_VERSION_PORTABLE/g" {} \; > /dev/null 2>&1
	fi
	if [ $PORTABLEAPPS -eq 1 ]
	then
		find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_VERSION_PORTABLEAPPS/#define ULTRACOPIER_VERSION_PORTABLEAPPS/g" {} \; > /dev/null 2>&1
	else
		find ${TEMP_PATH}/${TARGET}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_PORTABLEAPPS/\/\/#define ULTRACOPIER_VERSION_PORTABLEAPPS/g" {} \; > /dev/null 2>&1
	fi

	if [ ${STATIC} -eq 1 ]
	then
		if [ ${BITS} -eq 32 ]
		then
			find ${TEMP_PATH}/${TARGET}/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>windows-x86<\/architecture>/g" {} \; > /dev/null 2>&1
			REAL_WINEPREFIX="${WINEBASEPATH}/qt-5.0-32Bits-static-for-ultracopier/"
		fi
	        if [ ${BITS} -eq 64 ]
	        then
	                find ${TEMP_PATH}/${TARGET}/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>windows-x86_64<\/architecture>/g" {} \; > /dev/null 2>&1
	                REAL_WINEPREFIX="${WINEBASEPATH}/qt-5.0-64Bits-static-for-ultracopier/"
	        fi
	else
		if [ ${BITS} -eq 32 ]
		then
			find ${TEMP_PATH}/${TARGET}/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>windows-x86<\/architecture>/g" {} \; > /dev/null 2>&1
			REAL_WINEPREFIX="${WINEBASEPATH}/qt-5.0-32Bits-for-ultracopier/"
		fi
		if [ ${BITS} -eq 64 ]
		then
			find ${TEMP_PATH}/${TARGET}/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>windows-x86_64<\/architecture>/g" {} \; > /dev/null 2>&1
			REAL_WINEPREFIX="${WINEBASEPATH}/qt-5.0-64Bits-for-ultracopier/"
		fi
	fi
	if [ ${DEBUG_REAL} -eq 1 ]
	then
		COMPIL_SUFFIX="debug"
		COMPIL_FOLDER="debug"
	else
		COMPIL_SUFFIX="release"
		COMPIL_FOLDER="release"
	fi
	cd ${TEMP_PATH}/${TARGET}/
	PLUGIN_FOLDER="${TEMP_PATH}/${TARGET}/plugins/"
	cd ${PLUGIN_FOLDER}
	for plugins_cat in `ls -1`
	do
		if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
		then
			cd ${PLUGIN_FOLDER}/${plugins_cat}/
			for plugins_name in `ls -1`
			do
				if [ -d ${plugins_name} ] &&  [ -f ${plugins_name}/informations.xml ] && [ "${plugins_name}" != "KDE4" ] && [ "${plugins_name}" != "dbus" ] && [ ! -f ${plugins_name}/*.dll ] && [ ! -f ${plugins_name}/*.a ]
				then
					echo "${TARGET} compilation of the plugin: ${plugins_cat}/${plugins_name}..."
					cd ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/

					if [ ${STATIC} -ne 1 ]
					then
						cp ${BASE_PWD}/data/windows/resources-windows-ultracopier-plugins.rc ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/
						echo '' >> *.pro
						echo 'win32:RC_FILE += resources-windows-ultracopier-plugins.rc' >> *.pro
						# replace ULTRACOPIER_PLUGIN_VERSION
						ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
						sed -i "s/ULTRACOPIER_PLUGIN_VERSION/${ULTRACOPIER_PLUGIN_VERSION}/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
						# replace ULTRACOPIER_PLUGIN_WINDOWS_VERSION
						ULTRACOPIER_PLUGIN_WINDOWS_VERSION=`echo ${ULTRACOPIER_PLUGIN_VERSION} | sed "s/\./,/g"`
						sed -i "s/ULTRACOPIER_PLUGIN_WINDOWS_VERSION/${ULTRACOPIER_PLUGIN_WINDOWS_VERSION}/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
						# replace ULTRACOPIER_PLUGIN_NAME
						sed -i "s/ULTRACOPIER_PLUGIN_NAME/${plugins_name}/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
						# replace ULTRACOPIER_PLUGIN_FILENAME
						ULTRACOPIER_PLUGIN_FILENAME=`grep -F "qtLibraryTarget" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/*.pro | sed -r "s/^.*\((.*)\).*$/\1/g"`
						sed -i "s/ULTRACOPIER_PLUGIN_FILENAME/${ULTRACOPIER_PLUGIN_FILENAME}.dll/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
					fi

					DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 19 /usr/bin/ionice -c 3 wine qmake QMAKE_CFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CFLAGS="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" *.pro
					DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 19 /usr/bin/ionice -c 3 wine mingw32-make -j5 ${COMPIL_SUFFIX} > /dev/null 2>&1
					if [ ! -f ${COMPIL_FOLDER}/*.dll ] && [ ! -f ${COMPIL_FOLDER}/*.a ]
					then
						DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 19 /usr/bin/ionice -c 3 wine mingw32-make -j5 ${COMPIL_SUFFIX}
						echo "plugins not created"
						exit
					fi
					if [ ${STATIC} -eq 1 ]
					then
						if [ "${COMPIL_FOLDER}" != "./" ]
						then
							cp ${COMPIL_FOLDER}/*.a ./
						fi
					else
                                                if [ "${COMPIL_FOLDER}" != "./" ]
                                                then
                                                        mv ${COMPIL_FOLDER}/*.dll ./
                                                fi
					fi
					if [ $STATIC -ne 1 ]
					then
						/usr/bin/find ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/ -type f -name "*.png" -exec rm -f {} \;
					fi
				fi
				cd ${PLUGIN_FOLDER}/${plugins_cat}/
			done
			cd ${PLUGIN_FOLDER}/
		fi
	done
	if [ $ULTIMATE -eq 1 ] || [ $FORPLUGIN -eq 1 ]
	then
		PLUGIN_FOLDER="${TEMP_PATH}/${TARGET}/plugins-alternative/"
		cd ${PLUGIN_FOLDER}/
		for plugins_cat in `ls -1`
		do
			if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
			then
				cd ${PLUGIN_FOLDER}/${plugins_cat}/
				for plugins_name in `ls -1`
				do
					if [ -d ${plugins_name} ] &&  [ -f ${plugins_name}/informations.xml ] && [ ! -f ${plugins_name}/*.dll ] && [ ! -f ${plugins_name}/*.a ]
					then
						echo "${TARGET} compilation of the plugin: ${plugins_cat}/${plugins_name}..."
						cd ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/

						if [ ${STATIC} -ne 1 ]
						then
							cp ${BASE_PWD}/data/windows/resources-windows-ultracopier-plugins.rc ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/
							echo '' >> *.pro
							echo 'win32:RC_FILE += resources-windows-ultracopier-plugins.rc' >> *.pro
							# replace ULTRACOPIER_PLUGIN_VERSION
							ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
							sed -i "s/ULTRACOPIER_PLUGIN_VERSION/${ULTRACOPIER_PLUGIN_VERSION}/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
							# replace ULTRACOPIER_PLUGIN_WINDOWS_VERSION
							ULTRACOPIER_PLUGIN_WINDOWS_VERSION=`echo ${ULTRACOPIER_PLUGIN_VERSION} | sed "s/\./,/g"`
							sed -i "s/ULTRACOPIER_PLUGIN_WINDOWS_VERSION/${ULTRACOPIER_PLUGIN_WINDOWS_VERSION}/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
							# replace ULTRACOPIER_PLUGIN_NAME
							sed -i "s/ULTRACOPIER_PLUGIN_NAME/${plugins_name}/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
							# replace ULTRACOPIER_PLUGIN_FILENAME
							ULTRACOPIER_PLUGIN_FILENAME=`grep -F "qtLibraryTarget" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/*.pro | sed -r "s/^.*\((.*)\).*$/\1/g"`
							sed -i "s/ULTRACOPIER_PLUGIN_FILENAME/${ULTRACOPIER_PLUGIN_FILENAME}.dll/g" ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/resources-windows-ultracopier-plugins.rc
						fi

						DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 19 /usr/bin/ionice -c 3 wine qmake QMAKE_CFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CFLAGS="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" *.pro > /dev/null 2>&1
						DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 19 /usr/bin/ionice -c 3 wine mingw32-make -j5 ${COMPIL_SUFFIX} > /dev/null 2>&1
						if [ ! -f ${COMPIL_FOLDER}/*.dll ] && [ ! -f ${COMPIL_FOLDER}/*.a ]
						then
							DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 19 /usr/bin/ionice -c 3 wine mingw32-make -j5 ${COMPIL_SUFFIX}
							echo "plugins not created: ${plugins_cat}/${plugins_name}"
						else
							if [ ${STATIC} -eq 1 ]
							then
								if [ "${COMPIL_FOLDER}" != "./" ]
								then
									cp ${COMPIL_FOLDER}/*.a ./
								fi
							else
								if [ "${COMPIL_FOLDER}" != "./" ]
	                                                        then
	                                                                mv ${COMPIL_FOLDER}/*.dll ./
	                                                        fi
							fi
						fi
						if [ $STATIC -ne 1 ]
						then
							/usr/bin/find ${PLUGIN_FOLDER}/${plugins_cat}/${plugins_name}/ -type f -name "*.png" -exec rm -f {} \;
						fi
					fi
					cd ${PLUGIN_FOLDER}/${plugins_cat}/
				done
				cd ${PLUGIN_FOLDER}/
			fi
		done
	else
		rm -Rf ${TEMP_PATH}/${TARGET}/plugins-alternative/
	fi
	if [ ${STATIC} -eq 1 ]
	then
		cp ${TEMP_PATH}/${TARGET}/*/*/*/*/*.a ${TEMP_PATH}/${TARGET}/plugins/ > /dev/null 2>&1
	fi
	cd ${TEMP_PATH}/${TARGET}/
	if [ ! -f ultracopier.exe ]
	then
		if [ ${STATIC} -eq 1 ]
		then
			echo "${TARGET} static application..."
			DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 19 /usr/bin/ionice -c 3 wine qmake QMAKE_CFLAGS_RELEASE+="${CFLAGSCUSTOM}" QMAKE_CFLAGS+="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" ultracopier-static.pro > /dev/null 2>&1
		else
			echo "${TARGET} application..."
			DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 19 /usr/bin/ionice -c 3 wine qmake QMAKE_CFLAGS_RELEASE+="${CFLAGSCUSTOM}" QMAKE_CFLAGS+="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" ultracopier-core.pro > /dev/null 2>&1
		fi
		DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 19 /usr/bin/ionice -c 3 wine mingw32-make -j5 ${COMPIL_SUFFIX} > /dev/null 2>&1
		if [ ! -f ${COMPIL_FOLDER}/ultracopier.exe ]
		then
        	        if [ ${STATIC} -eq 1 ]
	                then
        	                DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 19 /usr/bin/ionice -c 3 wine qmake QMAKE_CFLAGS_RELEASE+="${CFLAGSCUSTOM}" QMAKE_CFLAGS+="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" ultracopier-static.pro
	                else
        	                DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 19 /usr/bin/ionice -c 3 wine qmake QMAKE_CFLAGS_RELEASE+="${CFLAGSCUSTOM}" QMAKE_CFLAGS+="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS_RELEASE="${CFLAGSCUSTOM}" QMAKE_CXXFLAGS="${CFLAGSCUSTOM}" ultracopier-core.pro
	                fi
			DISPLAY="na" WINEPREFIX=${REAL_WINEPREFIX} /usr/bin/nice -n 19 /usr/bin/ionice -c 3 wine mingw32-make -j5 ${COMPIL_SUFFIX}
			echo "application not created"
			exit
		fi
		if [ "${COMPIL_FOLDER}" != "./" ]
		then
			mv ${COMPIL_FOLDER}/ultracopier.exe ./
		fi
		if [ ${BITS} -eq 32 ]
		then
			upx --lzma -9 ultracopier.exe > /dev/null 2>&1
		fi
	fi
	/usr/bin/find ${TEMP_PATH}/${TARGET}/ -type f -not \( -name "*.xml" -or -name "*.dll" -or -name "*.a" -or -name "*.exe" -or -name "*.txt" -or -name "*.qm" \) -exec rm -f {} \;
	rm -Rf ${TEMP_PATH}/${TARGET}/resources/ ${PLUGIN_FOLDER}/SessionLoader/KDE4/
	rm -Rf ${TEMP_PATH}/${TARGET}/resources/ ${PLUGIN_FOLDER}/Listener/dbus/
	find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
	find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
	find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
	find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
	find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
	echo "${TARGET} compilation... done"
} 
