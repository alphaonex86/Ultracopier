#!/bin/bash

function assemble {
	TARGET=$1
	ARCHITECTURE=$2
	DEBUG=$3
	DEBUG_REAL=$4
	PORTABLE=$5
	ULTIMATE=$6
	STATIC=${7}
	ULTRACOPIER_VERSION_FINAL=${ULTRACOPIER_VERSION}
    DEBUGANDSETUP=0
	cd ${TEMP_PATH}/
    if [ ${DEBUG_REAL} -eq 1 ]
    then
        FINAL_ARCHIVE="${TARGET}-windows-${ARCHITECTURE}-${ULTRACOPIER_VERSION_FINAL}.7z"
    else
        FINAL_ARCHIVE="${TARGET}-windows-${ARCHITECTURE}-${ULTRACOPIER_VERSION_FINAL}.zip"
    fi
	if [ ! -d ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/ ]
	then
		echo "no previous compilation folder found into ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/..."
		exit
	fi
        if [ ! -e ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/*.exe ]
        then
                echo "no application found into ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/..."
                exit
        fi
	if [ ! -e ${FINAL_ARCHIVE} ]; then
		echo "creating the archive ${TARGET}..."
		if [ ${STATIC} -ne 1 ]
		then
			mkdir -p ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/CopyEngine/Ultracopier/
			mkdir -p ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Languages/
			mkdir -p ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Listener/catchcopy-v0002/
			mkdir -p ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/PluginLoader/catchcopy-v0002/
			mkdir -p ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/SessionLoader/Windows/
			mkdir -p ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Themes/Oxygen/

			if [ -e ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/plugins/ ]
			then
				if [ ${ULTIMATE} -eq 1 ]
				then
					rsync -aqrt ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/plugins/ ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/
					rsync -aqrt ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/plugins-alternative/ ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/
				else
					rsync -aqrt ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/plugins/ ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/
				fi
			fi

			rsync -aqrt ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/plugins/Languages/ ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Languages/
			rsync -aqrt ${ULTRACOPIERSOURCESPATH}/plugins/Languages/ ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Languages/ --exclude=*.xml --exclude=*.qm
			rsync -aqrt ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/plugins/CopyEngine/Ultracopier/Languages/ ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/CopyEngine/Ultracopier/Languages/
			rsync -aqrt ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/plugins/Themes/Oxygen/Languages/ ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Themes/Oxygen/Languages/
			find ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/ -iname "*.a" -exec rm {} \; > /dev/null 2>&1
			rm -Rf ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/plugins/ ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/plugins-alternative/
		else
			find ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/ -mindepth 1 -type d -exec rm -Rf {} \;
		fi
		cp -Rf ${ULTRACOPIERSOURCESPATH}/README ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/README.txt
		cp -Rf ${ULTRACOPIERSOURCESPATH}/COPYING ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/COPYING.txt
        if [ 1 == 2 ]
        then
            if [ "${ARCHITECTURE}" == "x86" ] && [ ${DEBUG_REAL} -ne 1 ]
            then
                upx --lzma -9 ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/ultracopier.exe > /dev/null 2>&1
            fi
        fi
		cp -Rf ${BASE_PWD}/data/windows-${ARCHITECTURE}/dll-qt-debug/lib* ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/
		if [ ${STATIC} -ne 1 ]
		then
			if [ ${DEBUG_REAL} -eq 1 ]
			then
				cp -Rf ${BASE_PWD}/data/windows-${ARCHITECTURE}/dll-qt-debug/* ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/
			else
				cp -Rf ${BASE_PWD}/data/windows-${ARCHITECTURE}/dll-qt/* ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/
			fi
		fi
		if [ ${DEBUG} -eq 1 ]
		then
			CATCHCOPY_SUFIX="d"
		else
			CATCHCOPY_SUFIX=""
		fi
		if [ ${STATIC} -eq 1 ]
		then
			cp -Rf ${BASE_PWD}/data/windows/catchcopy32${CATCHCOPY_SUFIX}.dll ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/catchcopy32${CATCHCOPY_SUFIX}.dll
			cp -Rf ${BASE_PWD}/data/windows/catchcopy64${CATCHCOPY_SUFIX}.dll ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/catchcopy64${CATCHCOPY_SUFIX}.dll
		else
			cp -Rf ${BASE_PWD}/data/windows/catchcopy32${CATCHCOPY_SUFIX}.dll ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/PluginLoader/catchcopy-v0002/catchcopy32${CATCHCOPY_SUFIX}.dll
			cp -Rf ${BASE_PWD}/data/windows/catchcopy64${CATCHCOPY_SUFIX}.dll ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/PluginLoader/catchcopy-v0002/catchcopy64${CATCHCOPY_SUFIX}.dll
		fi
		if [ ${STATIC} -ne 1 ]
		then
			cp -f ${BASE_PWD}/data/qm-translation/fr.qm ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Languages/fr/qt.qm
			cp -f ${BASE_PWD}/data/qm-translation/ar.qm ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Languages/ar/qt.qm
			cp -f ${BASE_PWD}/data/qm-translation/es.qm ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Languages/es/qt.qm
			cp -f ${BASE_PWD}/data/qm-translation/ja.qm ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Languages/ja/qt.qm
			cp -f ${BASE_PWD}/data/qm-translation/ko.qm ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Languages/ko/qt.qm
			cp -f ${BASE_PWD}/data/qm-translation/pl.qm ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Languages/pl/qt.qm
			cp -f ${BASE_PWD}/data/qm-translation/pt.qm ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Languages/pt/qt.qm
			cp -f ${BASE_PWD}/data/qm-translation/ru.qm ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Languages/ru/qt.qm
		fi
		find ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/ -iname "*.ts" -exec rm {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>windows-${ARCHITECTURE}<\/architecture>/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/ -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${ULTRACOPIER_VERSION_FINAL}<\/version>/g" {} \; > /dev/null 2>&1

		rm -Rf ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/SessionLoader/KDE4/
		rm -Rf ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/Listener/dbus/
		if [ ${PORTABLE} -eq 1 ]
		then
			rm -Rf ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/SessionLoader/
		fi
		find ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/ -type d -empty -delete > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/ -type d -empty -delete > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/ -type d -empty -delete > /dev/null 2>&1

        if [ ${DEBUG_REAL} -eq 1 ]
        then
            mkdir ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/ultracopier/
            mv ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/* ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/ultracopier/
            mv ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/ultracopier/ultracopier-debug.bat ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/
            cp -Rf /home/wine/qt-5.0-32Bits-for-ultracopier/drive_c/mingw32/ ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/mingw32/
        fi

	find ${TARGET}-windows-${ARCHITECTURE}/ -type d -exec chmod 700 "{}" \;
	find ${TARGET}-windows-${ARCHITECTURE}/ -type f -exec chmod 600 "{}" \;
	chown -Rf root.root ${TARGET}-windows-${ARCHITECTURE}/
	find ${TARGET}-windows-${ARCHITECTURE}/ -type f -exec touch -t 201601020000.00 "{}" \;
	find ${TARGET}-windows-${ARCHITECTURE}/ -type d -exec touch -t 201601020000.00 "{}" \;
        if [ ${DEBUG_REAL} -eq 1 ]
        then
            7za a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on ${FINAL_ARCHIVE} ${TARGET}-windows-${ARCHITECTURE}/
        else
            zip -r -q -9 ${FINAL_ARCHIVE} ${TARGET}-windows-${ARCHITECTURE}/
        fi
		#7za a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on ${FINAL_ARCHIVE} ${TARGET}-windows-${ARCHITECTURE}/
		#nice -n 15 ionice -c 3 tar cf - ${TARGET}-windows-${ARCHITECTURE}/ --owner=0 --group=0 --mtime='2010-01-01' -H ustar | nice -n 15 ionice -c 3 xz -z -9 -e > ${FINAL_ARCHIVE}
		if [ ! -e ${FINAL_ARCHIVE} ]; then
			echo "${FINAL_ARCHIVE} not exists!";
			exit;
		fi
		echo "creating the archive ${TARGET}... done"
	fi
	FINAL_ARCHIVE="${TARGET}-windows-${ARCHITECTURE}-${ULTRACOPIER_VERSION_FINAL}-setup.exe"
	if ( [ ${DEBUG} -eq 0 ] || [ ${DEBUGANDSETUP} -eq 1 ] ) && [ ${PORTABLE} -eq 0 ] && [ ! -e ${FINAL_ARCHIVE} ]; then
		echo "creating the installer ${TARGET}..."
		cd ${TEMP_PATH}/
		rm -Rf ${TEMP_PATH}/Ultracopier-installer-windows-${ARCHITECTURE}/
		mkdir -p ${TEMP_PATH}/Ultracopier-installer-windows-${ARCHITECTURE}/
		cd ${TEMP_PATH}/Ultracopier-installer-windows-${ARCHITECTURE}/
		if [ ${STATIC} -eq 1 ]
		then
			cp -aRf ${BASE_PWD}/data/windows/install-static.nsi ${TEMP_PATH}/Ultracopier-installer-windows-${ARCHITECTURE}/
		else
			cp -aRf ${BASE_PWD}/data/windows/install.nsi ${TEMP_PATH}/Ultracopier-installer-windows-${ARCHITECTURE}/
		fi
		#cp -aRf ${BASE_PWD}/data/windows/ultracopier.ico ${TEMP_PATH}/Ultracopier-installer-windows-${ARCHITECTURE}/
		rsync -art ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/ ${TEMP_PATH}/Ultracopier-installer-windows-${ARCHITECTURE}/
		cd ${TEMP_PATH}/Ultracopier-installer-windows-${ARCHITECTURE}/
		sed -i -r "s/X.X.X.X/${ULTRACOPIER_VERSION_FINAL}/g" *.nsi > /dev/null 2>&1
		if [ "${ARCHITECTURE}" != "x86" ]
		then
			sed -i -r "s/PROGRAMFILES/PROGRAMFILES64/g" *.nsi > /dev/null 2>&1
		fi
        if [ ${DEBUG} -eq 1 ]
        then
            sed -i -r "s/catchcopy32.dll/catchcopy32d.dll/g" *.nsi > /dev/null 2>&1
            sed -i -r "s/catchcopy64.dll/catchcopy64d.dll/g" *.nsi > /dev/null 2>&1
        fi
        DISPLAY="na" WINEPREFIX="${WINEBASEPATH}/ultracopier-general/" /usr/bin/nice -n 15 /usr/bin/ionice -c 3 wine "${WINEBASEPATH}/ultracopier-general/drive_c/Program Files (x86)/NSIS/makensis.exe" *.nsi > /dev/null 2>&1
		if [ ! -e *setup.exe ]; then
			echo "${TEMP_PATH}/${FINAL_ARCHIVE} not exists!";
			pwd
			exit;
		fi
		mv *setup.exe ${TEMP_PATH}/${FINAL_ARCHIVE}
                cd ${TEMP_PATH}/
		echo "creating the installer ${TARGET}... done"
	fi
	rm -Rf ${TEMP_PATH}/${TARGET}-windows-${ARCHITECTURE}/
}
