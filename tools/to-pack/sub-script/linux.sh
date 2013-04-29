#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

QMAKE="/usr/local/Qt-5.0.2/bin/qmake"

mkdir -p ${TEMP_PATH}
cd ${TEMP_PATH}/

function compil {
	DEBUG=$2
	ULTIMATE=$3
	cd ${TEMP_PATH}/
	TARGET=$1
	STATIC=$4
	FINAL_ARCHIVE="${TARGET}-linux-x86_64-pc-${ULTRACOPIER_VERSION}"
	if [ ! -e ${FINAL_ARCHIVE}.tar.xz ]
	then
		echo "Making linux tar.xz: ${FINAL_ARCHIVE} ..."

		rm -Rf ${TEMP_PATH}/${FINAL_ARCHIVE}/
		/usr/bin/rsync -art --delete ${ULTRACOPIER_SOURCE}/ ${TEMP_PATH}/${FINAL_ARCHIVE}/ --exclude='*build*' --exclude='*Qt_5*' --exclude='*qt5*' --exclude='*.pro.user'
		find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>linux-x86_64-pc<\/architecture>/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${ULTRACOPIER_VERSION}<\/version>/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_PORTABLE/\/\/#define ULTRACOPIER_VERSION_PORTABLE/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_PORTABLEAPPS/\/\/#define ULTRACOPIER_VERSION_PORTABLEAPPS/g" {} \; > /dev/null 2>&1
		if [ ${DEBUG} -eq 1 ]
		then
			find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_DEBUG/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
			find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
			find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
		else
			find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_DEBUG/\/\/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
			find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
			find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
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

		if [ ${DEBUG} -eq 1 ]
		then
			QTMODEDEBUGRELEASE="debug"
		else
			QTMODEDEBUGRELEASE="release"
		fi
		cd ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins/CopyEngine/Ultracopier/
		${QMAKE} -config ${QTMODEDEBUGRELEASE}
		make -j 5 > /dev/null 2>&1
		RETURN_CODE=$?
		if [ $? -ne 0 ]
		then
			echo "make failed on the linux: ${RETURN_CODE}"
			exit
		fi
		cd ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins/Listener/catchcopy-v0002/
		${QMAKE} -config ${QTMODEDEBUGRELEASE}
		make -j 5 > /dev/null 2>&1
		RETURN_CODE=$?
		if [ $? -ne 0 ]
		then
			echo "make failed on the linux: ${RETURN_CODE}"
			exit
		fi
		cd ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins/Themes/Oxygen/
		${QMAKE} -config ${QTMODEDEBUGRELEASE}
		make -j 5 > /dev/null 2>&1
		RETURN_CODE=$?
		if [ $? -ne 0 ]
		then
			echo "make failed on the linux: ${RETURN_CODE}"
			exit
		fi
		if [ ${ULTIMATE} -eq 1 ] && [ $STATIC -ne 1 ]
		then
			cd ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins-alternative/Themes/Clean/
			${QMAKE} -config ${QTMODEDEBUGRELEASE}
			make -j 5 > /dev/null 2>&1
			RETURN_CODE=$?
			if [ $? -ne 0 ]
			then
				echo "make failed on the linux: ${RETURN_CODE}"
				exit
			fi
			mv ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins-alternative/Themes/Clean/ ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins/Themes/Clean/
			cd ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins-alternative/Themes/Windows/
			${QMAKE} -config ${QTMODEDEBUGRELEASE}
			make -j 5 > /dev/null 2>&1
			RETURN_CODE=$?
			if [ $? -ne 0 ]
			then
				echo "make failed on the linux: ${RETURN_CODE}"
				exit
			fi
			mv ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins-alternative/Themes/Windows/ ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins/Themes/Windows/
			cd ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins-alternative/Themes/Teracopy/
			${QMAKE} -config ${QTMODEDEBUGRELEASE}
			make -j 5 > /dev/null 2>&1
			RETURN_CODE=$?
			if [ $? -ne 0 ]
			then
				echo "make failed on the linux: ${RETURN_CODE}"
				exit
			fi
			mv ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins-alternative/Themes/Teracopy/ ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins/Themes/Teracopy/
# 			if [ $STATIC -eq 1 ]
# 			then
# 				cp -aRf ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins-alternative/*/*/*.a ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins/
# 			fi
		else
			rm -Rf ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins-alternative/
		fi
		cd ${TEMP_PATH}/${FINAL_ARCHIVE}/
		if [ $STATIC -eq 1 ]
		then
			cp -aRf ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins/*/*/*.a ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins/
			${QMAKE} -config ${QTMODEDEBUGRELEASE} ultracopier-static.pro
		else
			${QMAKE} -config ${QTMODEDEBUGRELEASE} ultracopier-core.pro
		fi
		make -j 5 > /dev/null 2>&1
		if [ $STATIC -eq 1 ]
		then
			upx --lzma -9 ultracopier > /dev/null 2>&1
		fi
		RETURN_CODE=$?
		if [ $? -ne 0 ] || [ ! -e ultracopier ]
		then
			echo "make failed on the linux: ${RETURN_CODE}"
			exit
		fi
		cd ${TEMP_PATH}/${FINAL_ARCHIVE}
		for SUBFOLDER in `ls -1`
		do
			if [ -d ${TEMP_PATH}/${FINAL_ARCHIVE}/${SUBFOLDER}/ ]
			then
				cd ${TEMP_PATH}/${FINAL_ARCHIVE}/${SUBFOLDER}/
				for plugins_cat in `ls -1`
				do
					if [ -d ${TEMP_PATH}/${FINAL_ARCHIVE}/${SUBFOLDER}/${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
					then
						cd ${TEMP_PATH}/${FINAL_ARCHIVE}/${SUBFOLDER}/${plugins_cat}/
						for plugins_name in `ls -1`
						do
							if [ ! -f ${plugins_name}/lib*.so ] && [ -d ${plugins_name}/ ]
							then
								rm -Rf ${TEMP_PATH}/${FINAL_ARCHIVE}/${SUBFOLDER}/${plugins_cat}/${plugins_name}/
							fi
						done
					fi
				done
			fi
		done
		cd ${TEMP_PATH}/
		rm -Rf ${TEMP_PATH}/${FINAL_ARCHIVE}/resources/
		if [ $STATIC -eq 1 ]
		then
			rm -Rf ${TEMP_PATH}/${FINAL_ARCHIVE}/plugins/
		fi
		/usr/bin/find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -type f -not \( -name "*.xml" -or -name "lib*.so" -or -name "ultracopier" -or -name "*.txt" -or -name "*.qm" \) -exec rm -f {} \;
		/usr/bin/find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -type d \( -name "*build*" -or -name "Desktop" -or -name "Qt_5" -or -name "qt5" \) -exec rm -Rf {} \;
		/usr/bin/find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -type d -empty -delete > /dev/null 2>&1
		/usr/bin/find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -type d -empty -delete > /dev/null 2>&1
		/usr/bin/find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -type d -empty -delete > /dev/null 2>&1
		/usr/bin/find ${TEMP_PATH}/${FINAL_ARCHIVE}/ -type d -empty -delete > /dev/null 2>&1
		cd ${TEMP_PATH}/
		nice -n 19 ionice -c 3 tar cpf - ${FINAL_ARCHIVE}/ | nice -n 19 ionice -c 3 xz -z -9 -e > ${FINAL_ARCHIVE}.tar.xz
		if [ ! -e ${FINAL_ARCHIVE}.tar.xz ]; then
			echo "${FINAL_ARCHIVE}.tar.xz not exists!";
			exit;
		fi
		echo "Making binary debug linux tar.xz... done"
	else
		echo "Archive already exists: ${FINAL_ARCHIVE}.tar.xz"
	fi
}

function compil_plugin {
	DEBUG=$2
	cd ${TEMP_PATH}/
	TARGET=$1
	SUBFOLDER=$3
	FINAL_ARCHIVE="${TARGET}-linux-x86_64-pc-for-plugins"

	cp -aRf ${ULTRACOPIER_SOURCE}/ ${TEMP_PATH}/${FINAL_ARCHIVE}/

	if [ ${DEBUG} -eq 1 ]
	then
		QTMODEDEBUGRELEASE="debug"
	else
		QTMODEDEBUGRELEASE="release"
	fi

	cd ${TEMP_PATH}/${FINAL_ARCHIVE}/${SUBFOLDER}/
	for plugins_cat in `ls -1`
	do
		if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
		then
			cd ${TEMP_PATH}/${FINAL_ARCHIVE}/${SUBFOLDER}/${plugins_cat}/
			for plugins_name in `ls -1`
			do
				if [ -f ${plugins_name}/informations.xml ]
				then
					find ${plugins_name}/ -name "informations.xml" -exec sed -i -r "s/1\.0\.0\.0/${ULTRACOPIER_VERSION}/g" {} \; > /dev/null 2>&1
					ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
					if [ -d ${plugins_name} ] && [ ! -f ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-linux-x86_64-pc.urc ]
					then
						echo "pack the ${ARCHITECTURE} linux for the alternative plugin: ${plugins_cat}/${plugins_name}"
						mkdir -p ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/

						find ${plugins_name}/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
						find ${plugins_name}/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
						find ${plugins_name}/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>linux-x86_64-pc<\/architecture>/g" {} \; > /dev/null 2>&1
						find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_PORTABLE/\/\/#define ULTRACOPIER_VERSION_PORTABLE/g" {} \; > /dev/null 2>&1
						find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_PORTABLEAPPS/\/\/#define ULTRACOPIER_VERSION_PORTABLEAPPS/g" {} \; > /dev/null 2>&1
						if [ ${DEBUG} -eq 1 ]
						then
							find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_DEBUG/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
							find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
							find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
						else
							find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_DEBUG/\/\/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
							find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
							find ${plugins_name}/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
						fi
						cd ${plugins_name}/
						${QMAKE} -config ${QTMODEDEBUGRELEASE} *.pro
						make -j 5 > /dev/null 2>&1
						cd ${TEMP_PATH}/${FINAL_ARCHIVE}/${SUBFOLDER}/${plugins_cat}/
						if [ ! -e ${plugins_name}/lib*.so ]; then
							echo "no lib*.so file!";
							make -j 5
						else
							find ${plugins_name}/ -iname "*.ts" -exec rm {} \;
							find ${plugins_name}/ -maxdepth 1 -mindepth 1 -type f ! -iname "lib*.so" ! -iname "informations.xml" -exec rm {} \;
							find ${plugins_name}/ -maxdepth 1 -mindepth 1 -type d ! -iname "Languages" -exec rm -Rf {} \;
							/usr/bin/find ${plugins_name}/ -type d -empty -delete > /dev/null 2>&1
							/usr/bin/find ${plugins_name}/ -type d -empty -delete > /dev/null 2>&1
							/usr/bin/find ${plugins_name}/ -type d -empty -delete > /dev/null 2>&1
							/usr/bin/find ${plugins_name}/ -type d -empty -delete > /dev/null 2>&1

							tar --posix -c -f - ${plugins_name}/ | xz -9 --check=crc32 > ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-linux-x86_64-pc.urc
						fi
					fi
				fi
			done
		fi
		cd ${TEMP_PATH}/${FINAL_ARCHIVE}/${SUBFOLDER}/
	done
}

compil "ultracopier" 0 0 1
compil "ultracopier-ultimate" 0 1 1
compil "ultracopier-debug" 1 0 1

#compil_plugin "ultracopier" 0 "plugins-alternative"
#compil_plugin "ultracopier" 0 "plugins"

