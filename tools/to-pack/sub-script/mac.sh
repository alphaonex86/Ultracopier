#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

mkdir -p ${TEMP_PATH}
cd ${TEMP_PATH}/

IPMAC="192.168.158.34"
SSHUSER="user"
QTVERSION="5.4.0"
QTVERSIONMAJ="5.4"

ssh root@${IPMAC} "cd /usr/bin/;ln -s /Applications/Xcode.app/Contents/Developer//Toolchains/XcodeDefault.xctoolchain/usr/bin/otool"
ssh root@${IPMAC} "mkdir /opt/;mkdir /opt/local/;mkdir /opt/local/lib/;cd /opt/local/lib/;ln -s /usr/lib/libz.1.dylib"

#compil "ultracopier" 1 0
function compil {
	DEBUG=$2
	ULTIMATE=$3
	cd ${TEMP_PATH}/
	TARGET=$1
	STATIC=$4
	DEBUGREAL=${5}
	if [ $DEBUGREAL -eq 1 ]
	then
		LIBEXT="_debug.dylib"
		CONFIGARG="debug"
	else
		LIBEXT=".dylib"
		CONFIGARG="release"
	fi
	FINAL_ARCHIVE="${TARGET}-mac-os-x-${ULTRACOPIER_VERSION}.dmg"
	if [ ! -e ${FINAL_ARCHIVE} ]
	then
		echo "Making Mac dmg: ${FINAL_ARCHIVE} ..."

		rm -Rf ${TEMP_PATH}/${TARGET}-mac-os-x/
		cp -aRf ${ULTRACOPIER_SOURCE}/ ${TEMP_PATH}/${TARGET}-mac-os-x/
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>mac-os-x<\/architecture>/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${ULTRACOPIER_VERSION}<\/version>/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "informations.xml" -exec sed -i -r "s/<pubDate>.*<\pubDate>/<pubDate>`date +%s`<\pubDate>/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_PORTABLE/\/\/#define ULTRACOPIER_VERSION_PORTABLE/g" {} \; > /dev/null 2>&1
		find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_PORTABLEAPPS/\/\/#define ULTRACOPIER_VERSION_PORTABLEAPPS/g" {} \; > /dev/null 2>&1
		if [ ${DEBUG} -eq 1 ]
		then
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_DEBUG/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
		else
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_DEBUG/\/\/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
		fi
		if [ $STATIC -eq 1 ]
		then
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/g" {} \; > /dev/null 2>&1
		else

			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/\/\/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/g" {} \; > /dev/null 2>&1
		fi
		if [ $ULTIMATE -eq 1 ]
		then
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_VERSION_ULTIMATE/#define ULTRACOPIER_VERSION_ULTIMATE/g" {} \; > /dev/null 2>&1
		else
			find ${TEMP_PATH}/${TARGET}-mac-os-x/ -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_ULTIMATE/\/\/#define ULTRACOPIER_VERSION_ULTIMATE/g" {} \; > /dev/null 2>&1
		fi

		echo "try connect"
		ssh ${SSHUSER}@${IPMAC} "rm -fR /Users/${SSHUSER}/Desktop/ultracopier/"
		echo "try rsync"
		rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/ultracopier/

		echo "try qmake"
		BASEAPPNAME="ultracopier.app"
		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/;/Users/user/Qt${QTVERSION}/${QTVERSIONMAJ}/clang_64/bin/qmake /Users/${SSHUSER}/Desktop/ultracopier/ultracopier-core.pro -spec macx-g++ -config ${CONFIGARG}"
		echo "try make"
		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3 > /dev/null > /dev/null"
		RETURN_CODE=$?
		if [ $? -ne 0 ]
		then
			echo "make failed on the mac: ${RETURN_CODE}"
			exit
		fi
		echo "try make plugins"
		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/plugins/CopyEngine/Ultracopier/;/Users/user/Qt${QTVERSION}/${QTVERSIONMAJ}/clang_64/bin/qmake /Users/${SSHUSER}/Desktop/ultracopier/plugins/CopyEngine/Ultracopier/*.pro -spec macx-g++ -config ${CONFIGARG}"
		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/plugins/CopyEngine/Ultracopier/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3 > /dev/null"
		RETURN_CODE=$?
		if [ $? -ne 0 ]
		then
			echo "make failed on the mac: ${RETURN_CODE}"
			exit
		fi
		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/plugins/Listener/catchcopy-v0002/;/Users/user/Qt${QTVERSION}/${QTVERSIONMAJ}/clang_64/bin/qmake /Users/${SSHUSER}/Desktop/ultracopier/plugins/Listener/catchcopy-v0002/*.pro -spec macx-g++ -config ${CONFIGARG}"
		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/plugins/Listener/catchcopy-v0002/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3 > /dev/null"
		RETURN_CODE=$?
		if [ $? -ne 0 ]
		then
			echo "make failed on the mac: ${RETURN_CODE}"
			exit
		fi
		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/plugins/Themes/Oxygen/;/Users/user/Qt${QTVERSION}/${QTVERSIONMAJ}/clang_64/bin/qmake /Users/${SSHUSER}/Desktop/ultracopier/plugins/Themes/Oxygen/*.pro -spec macx-g++ -config ${CONFIGARG}"
		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/plugins/Themes/Oxygen/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3 > /dev/null"
		RETURN_CODE=$?
		if [ $? -ne 0 ]
		then
			echo "make failed on the mac: ${RETURN_CODE}"
			exit
		fi
		if [ ${ULTIMATE} -eq 1 ]
		then
			echo "do the ultimate plugin"
			ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/plugins-alternative/Themes/Windows/;/Users/user/Qt${QTVERSION}/${QTVERSIONMAJ}/clang_64/bin/qmake /Users/${SSHUSER}/Desktop/ultracopier/plugins-alternative/Themes/Windows/*.pro -spec macx-g++ -config ${CONFIGARG}" > /dev/null 2>&1
			ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/plugins-alternative/Themes/Windows/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3 > /dev/null"
			RETURN_CODE=$?
			if [ $? -ne 0 ]
			then
				echo "make failed on the mac: ${RETURN_CODE}"
				exit
			fi
			ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/plugins-alternative/Themes/Teracopy/;/Users/user/Qt${QTVERSION}/${QTVERSIONMAJ}/clang_64/bin/qmake /Users/${SSHUSER}/Desktop/ultracopier/plugins-alternative/Themes/Teracopy/*.pro -spec macx-g++ -config ${CONFIGARG}" > /dev/null 2>&1
			ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/plugins-alternative/Themes/Teracopy/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3 > /dev/null"
			RETURN_CODE=$?
			if [ $? -ne 0 ]
			then
				echo "make failed on the mac: ${RETURN_CODE}"
				exit
			fi
			ssh ${SSHUSER}@${IPMAC} "mkdir -p /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Windows/"
			ssh ${SSHUSER}@${IPMAC} "mkdir -p /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Teracopy/"
			ssh ${SSHUSER}@${IPMAC} "cp /Users/${SSHUSER}/Desktop/ultracopier/plugins-alternative/Themes/Windows/informations.xml /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Windows/informations.xml"
			ssh ${SSHUSER}@${IPMAC} "cp /Users/${SSHUSER}/Desktop/ultracopier/plugins-alternative/Themes/Teracopy/informations.xml /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Teracopy/informations.xml"
			ssh ${SSHUSER}@${IPMAC} "cp /Users/${SSHUSER}/Desktop/ultracopier/plugins-alternative/Themes/Windows/libinterface${LIBEXT} /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Windows/libinterface${LIBEXT}"
			ssh ${SSHUSER}@${IPMAC} "cp /Users/${SSHUSER}/Desktop/ultracopier/plugins-alternative/Themes/Teracopy/libinterface${LIBEXT} /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Teracopy/libinterface${LIBEXT}"
			ssh ${SSHUSER}@${IPMAC} "rsync -aqrt /Users/${SSHUSER}/Desktop/ultracopier/plugins-alternative/Themes/Windows/Languages/ /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Windows/Languages/"
			ssh ${SSHUSER}@${IPMAC} "rsync -aqrt /Users/${SSHUSER}/Desktop/ultracopier/plugins-alternative/Themes/Teracopy/Languages/ /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Teracopy/Languages/"
			ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Windows/libinterface${LIBEXT}"
			ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Windows/libinterface${LIBEXT}"
			ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtNetwork.framework/Versions/5/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Windows/libinterface${LIBEXT}"
			ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Teracopy/libinterface${LIBEXT}"
			ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Teracopy/libinterface${LIBEXT}"
			ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtNetwork.framework/Versions/5/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Teracopy/libinterface${LIBEXT}"
		fi
		echo "make the folder"
		ssh ${SSHUSER}@${IPMAC} "mkdir /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/Frameworks"

		ssh ${SSHUSER}@${IPMAC} "mkdir -p /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/CopyEngine/Ultracopier/"
		ssh ${SSHUSER}@${IPMAC} "mkdir -p /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Languages/"
		ssh ${SSHUSER}@${IPMAC} "mkdir -p /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Listener/catchcopy-v0002/"

		echo "put the informations.xml"
		ssh ${SSHUSER}@${IPMAC} "cp /Users/${SSHUSER}/Desktop/ultracopier/plugins/CopyEngine/Ultracopier/informations.xml /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/CopyEngine/Ultracopier/informations.xml"
		ssh ${SSHUSER}@${IPMAC} "cp /Users/${SSHUSER}/Desktop/ultracopier/plugins/Listener/catchcopy-v0002/informations.xml /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Listener/catchcopy-v0002/informations.xml"

		ssh ${SSHUSER}@${IPMAC} "cp /Users/${SSHUSER}/Desktop/ultracopier/plugins/CopyEngine/Ultracopier/libcopyEngine${LIBEXT} /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/CopyEngine/Ultracopier/libcopyEngine${LIBEXT}"
		ssh ${SSHUSER}@${IPMAC} "cp /Users/${SSHUSER}/Desktop/ultracopier/plugins/Listener/catchcopy-v0002/liblistener${LIBEXT} /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Listener/catchcopy-v0002/liblistener${LIBEXT}"

		ssh ${SSHUSER}@${IPMAC} "rsync -aqrt /Users/${SSHUSER}/Desktop/ultracopier/plugins/Languages/ /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Languages/"
		ssh ${SSHUSER}@${IPMAC} "rsync -aqrt /Users/${SSHUSER}/Desktop/ultracopier/plugins/CopyEngine/Ultracopier/Languages/ /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/CopyEngine/Ultracopier/Languages/"
		ssh ${SSHUSER}@${IPMAC} "find /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/ -iname \"*.ts\" -exec rm {} \; > /dev/null 2>&1"

		echo "finish the link"
		ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/CopyEngine/Ultracopier/libcopyEngine${LIBEXT}"
		ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/CopyEngine/Ultracopier/libcopyEngine${LIBEXT}"
		ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtNetwork.framework/Versions/5/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/CopyEngine/Ultracopier/libcopyEngine${LIBEXT}"
		ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Listener/catchcopy-v0002/liblistener${LIBEXT}"
		ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Listener/catchcopy-v0002/liblistener${LIBEXT}"
		ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtNetwork.framework/Versions/5/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Listener/catchcopy-v0002/liblistener${LIBEXT}"
		ssh ${SSHUSER}@${IPMAC} "mkdir -p /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Oxygen/"
		ssh ${SSHUSER}@${IPMAC} "cp /Users/${SSHUSER}/Desktop/ultracopier/plugins/Themes/Oxygen/informations.xml /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Oxygen/informations.xml"
		ssh ${SSHUSER}@${IPMAC} "cp /Users/${SSHUSER}/Desktop/ultracopier/plugins/Themes/Oxygen/libinterface${LIBEXT} /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Oxygen/libinterface${LIBEXT}"
		ssh ${SSHUSER}@${IPMAC} "rsync -aqrt /Users/${SSHUSER}/Desktop/ultracopier/plugins/Themes/Oxygen/Languages/ /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Oxygen/Languages/"
		ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Oxygen/libinterface${LIBEXT}"
		ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Oxygen/libinterface${LIBEXT}"
		ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtNetwork.framework/Versions/5/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork /Users/${SSHUSER}/Desktop/ultracopier/${BASEAPPNAME}/Contents/MacOS/Themes/Oxygen/libinterface${LIBEXT}"
		ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/;/Users/user/Qt${QTVERSION}/${QTVERSIONMAJ}/clang_64/bin/macdeployqt ${BASEAPPNAME}/ -dmg"
		rsync -art ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/ultracopier/ultracopier.dmg ${TEMP_PATH}/${FINAL_ARCHIVE}
		if [ ! -e ${FINAL_ARCHIVE} ]; then
			echo "${FINAL_ARCHIVE} not exists!";
			exit;
		fi
		ssh ${SSHUSER}@${IPMAC} "rm -fR /Users/${SSHUSER}/Desktop/ultracopier/"
		echo "Making binary debug Mac dmg... done"
	else
		echo "Archive already exists: ${FINAL_ARCHIVE}"
	fi
}

function compil_plugin {
	DEBUG=$2
	cd ${TEMP_PATH}/
	TARGET=$1
	SUBFOLDER=$3

	rsync -art --delete ${ULTRACOPIER_SOURCE}/ ${TEMP_PATH}/${TARGET}-mac-os-x/

	cd ${TEMP_PATH}/${TARGET}-mac-os-x/${SUBFOLDER}/
	for plugins_cat in `ls -1`
	do
		if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
		then
			cd ${TEMP_PATH}/${TARGET}-mac-os-x/${SUBFOLDER}/${plugins_cat}/
			for plugins_name in `ls -1`
			do
				if [ -f ${plugins_name}/informations.xml ]
				then
					find ${plugins_name}/ -name "informations.xml" -exec sed -i -r "s/1\.0\.0\.0/${ULTRACOPIER_VERSION}/g" {} \; > /dev/null 2>&1
					ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" ${plugins_name}/informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
					# && [ ${plugins_name} != "Windows" ] -> for the plugins interface
					if [ -d ${plugins_name} ] && [ ! -f "${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-mac-os-x.urc" ] && [ ${plugins_name} != "dbus" ] && [ ${plugins_cat} != "SessionLoader" ]
					then
						echo "pack the mac for the alternative plugin: ${plugins_cat}/${plugins_name}"
						mkdir -p ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/

						find ${plugins_name}/ -name "*.pro.user" -exec rm {} \; > /dev/null 2>&1
						find ${plugins_name}/ -name "*-build-desktop" -type d -exec rm -Rf {} \; > /dev/null 2>&1
						find ${plugins_name}/ -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>mac-os-x<\/architecture>/g" {} \; > /dev/null 2>&1
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

						ssh ${SSHUSER}@${IPMAC} "rm -fR /Users/${SSHUSER}/Desktop/ultracopier/"
						rsync -art ${TEMP_PATH}/${TARGET}-mac-os-x/ ${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/ultracopier/
						ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/${SUBFOLDER}/${plugins_cat}/${plugins_name}/;/Users/user/Qt${QTVERSION}/${QTVERSIONMAJ}/clang_64/bin/qmake /Users/${SSHUSER}/Desktop/ultracopier/${SUBFOLDER}/${plugins_cat}/${plugins_name}/*.pro -spec macx-g++ -config ${CONFIGARG}"
						ssh ${SSHUSER}@${IPMAC} "cd /Users/${SSHUSER}/Desktop/ultracopier/${SUBFOLDER}/${plugins_cat}/${plugins_name}/;/Applications/Xcode.app/Contents/Developer/usr/bin/gnumake -j 3 > /dev/null"
						RETURN_CODE=$?
						if [ $? -ne 0 ]
						then
							echo "make failed on the mac: ${RETURN_CODE}"
							exit
						fi
						ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui /Users/${SSHUSER}/Desktop/ultracopier/${SUBFOLDER}/${plugins_cat}/${plugins_name}/*${LIBEXT}"
						ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore /Users/${SSHUSER}/Desktop/ultracopier/${SUBFOLDER}/${plugins_cat}/${plugins_name}/*${LIBEXT}"
						ssh ${SSHUSER}@${IPMAC} "/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/install_name_tool -change QtNetwork.framework/Versions/5/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork /Users/${SSHUSER}/Desktop/ultracopier/${SUBFOLDER}/${plugins_cat}/${plugins_name}/*${LIBEXT}"
						rsync -art "${SSHUSER}@${IPMAC}:/Users/${SSHUSER}/Desktop/ultracopier/${SUBFOLDER}/${plugins_cat}/${plugins_name}/*${LIBEXT}" ${plugins_name}/
						if [ ! -e ${plugins_name}/*.dylib ]; then
							echo "no .dylib file!";
						else
							find ${plugins_name}/ -iname "*.ts" -exec rm {} \;
							find ${plugins_name}/ -maxdepth 1 -mindepth 1 -type f ! -iname "*.dylib" ! -iname "informations.xml" -exec rm {} \;
							find ${plugins_name}/ -maxdepth 1 -mindepth 1 -type d ! -iname "Languages" -exec rm -Rf {} \;
							/usr/bin/find ${plugins_name}/ -type d -empty -delete > /dev/null 2>&1
							/usr/bin/find ${plugins_name}/ -type d -empty -delete > /dev/null 2>&1
							/usr/bin/find ${plugins_name}/ -type d -empty -delete > /dev/null 2>&1
							/usr/bin/find ${plugins_name}/ -type d -empty -delete > /dev/null 2>&1

							tar --posix -c -f - ${plugins_name}/ | xz -9 --check=crc32 > ${TEMP_PATH}/plugins/${plugins_cat}/${plugins_name}/${plugins_cat}-${plugins_name}-${ULTRACOPIER_PLUGIN_VERSION}-mac-os-x.urc
						fi
					fi
				fi
			done
		fi
		cd ${TEMP_PATH}/${TARGET}-mac-os-x/${SUBFOLDER}/
	done
}

compil "ultracopier" 0 0 0 0 0
compil "ultracopier-ultimate" 0 1 0 0 0
compil "ultracopier-debug" 1 0 0 0 0

compil_plugin "ultracopier" 0 "plugins-alternative"
compil_plugin "ultracopier" 0 "plugins"
