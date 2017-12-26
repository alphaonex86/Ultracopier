#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi
if [ "${ULTRACOPIER_VERSION}" = "" ]
then
        exit;
fi
SUPERCOPIER_VERSION=`echo "${ULTRACOPIER_VERSION}"`

cd ${TEMP_PATH}/

echo "Move some elements..."
if [ -d ${TEMP_PATH}/doc/ ]
then
	rsync -artu ${TEMP_PATH}/doc/ /home/first-world.info/doc-ultracopier/
	if [ $? -ne 0 ]
	then
		echo 'rsync failed'
	        exit;
	fi
	rm -Rf ${TEMP_PATH}/doc/
fi
if [ -d ${TEMP_PATH}/plugins/ ]
then
	rsync -artu ${TEMP_PATH}/plugins/ /home/first-world.info/files-rw/ultracopier/plugins/
	if [ $? -ne 0 ]
	then
	        echo 'rsync failed'
	        exit;
	fi
	rm -Rf ${TEMP_PATH}/plugins/
fi

mkdir -p /home/first-world.info/files-rw/supercopier/${SUPERCOPIER_VERSION}/
mv ${TEMP_PATH}/supercopier-*.zip /home/first-world.info/files-rw/supercopier/${SUPERCOPIER_VERSION}/
mv ${TEMP_PATH}/supercopier-*-setup.exe /home/first-world.info/files-rw/supercopier/${SUPERCOPIER_VERSION}/

mkdir -p /home/first-world.info/files-rw/ultracopier/${ULTRACOPIER_VERSION}/
mv ${TEMP_PATH}/ultracopier-*.tar.xz /home/first-world.info/files-rw/ultracopier/${ULTRACOPIER_VERSION}/
mv ${TEMP_PATH}/ultracopier-*.zip /home/first-world.info/files-rw/ultracopier/${ULTRACOPIER_VERSION}/
mv ${TEMP_PATH}/ultracopier-*-setup.exe /home/first-world.info/files-rw/ultracopier/${ULTRACOPIER_VERSION}/
mv ${TEMP_PATH}/ultracopier-*.tar.bz2 /home/first-world.info/files-rw/ultracopier/${ULTRACOPIER_VERSION}/
cp ${BASE_PWD}/data/gentoo/ultracopier.ebuild /home/first-world.info/files-rw/ultracopier/${ULTRACOPIER_VERSION}/ultracopier-${ULTRACOPIER_VERSION}.ebuild
echo "Move some elements... done"

echo "Finalise some elements..."
chown lighttpd.lighttpd -Rf /home/first-world.info/doc-ultracopier/*/*/${ULTRACOPIER_VERSION}/
chown lighttpd.lighttpd -Rf /home/first-world.info/files-rw/ultracopier/plugins/
echo "Finalise some elements... done"

echo "Upload to the shop..."
#cd /home/first-world.info/files-rw/ultracopier/${ULTRACOPIER_VERSION}/ && rm -f /home/first-world.info/shop/download/09616e4ee15b7445c80704b934559f3af483abaa && nice -n 19 ionice -c 3 zip -9 -q /home/first-world.info/shop/download/09616e4ee15b7445c80704b934559f3af483abaa ultracopier-ultimate-*-x86-${ULTRACOPIER_VERSION}-setup.exe -x ultracopier-ultimate-cgminer-*-x86-${ULTRACOPIER_VERSION}-setup.exe && mv /home/first-world.info/shop/download/09616e4ee15b7445c80704b934559f3af483abaa.zip /home/first-world.info/shop/download/09616e4ee15b7445c80704b934559f3af483abaa
#cd /home/first-world.info/files-rw/ultracopier/${ULTRACOPIER_VERSION}/ && rm -f /home/first-world.info/shop/download/e4039d2bc194fb68f3ac6319c63381eed1573632 && nice -n 19 ionice -c 3 zip -9 -q /home/first-world.info/shop/download/e4039d2bc194fb68f3ac6319c63381eed1573632 ultracopier-ultimate-*-x86_64-${ULTRACOPIER_VERSION}-setup.exe -x ultracopier-ultimate-cgminer-*-x86_64-${ULTRACOPIER_VERSION}-setup.exe && mv /home/first-world.info/shop/download/e4039d2bc194fb68f3ac6319c63381eed1573632.zip /home/first-world.info/shop/download/e4039d2bc194fb68f3ac6319c63381eed1573632
#cp /home/first-world.info/files-rw/ultracopier/${ULTRACOPIER_VERSION}/ultracopier-ultimate-mac-os-x-${ULTRACOPIER_VERSION}.dmg /home/first-world.info/shop/download/d6382b673f31a42c71101ed642fe69d3b39dba8a
#cd /home/first-world.info/files-rw/supercopier/${SUPERCOPIER_VERSION}/ && rm -f /home/first-world.info/shop/download/b5f718420c697ddb34d3dfae6cb2adfdfc860153 && nice -n 19 ionice -c 3 zip -9 -q /home/first-world.info/shop/download/b5f718420c697ddb34d3dfae6cb2adfdfc860153 supercopier-ultimate-*${SUPERCOPIER_VERSION}* -x supercopier-ultimate-cgminer-*${SUPERCOPIER_VERSION}* && mv /home/first-world.info/shop/download/b5f718420c697ddb34d3dfae6cb2adfdfc860153.zip /home/first-world.info/shop/download/b5f718420c697ddb34d3dfae6cb2adfdfc860153
 
#cd /home/first-world.info/files-rw/ultracopier/plugins/Themes/Teracopy/ && rm -f /home/first-world.info/shop/download/161e15b3dfd41a1c4fc265d8d2d856a07e8df559 && nice -n 19 ionice -c 3 zip -9 -q /home/first-world.info/shop/download/161e15b3dfd41a1c4fc265d8d2d856a07e8df559 *${ULTRACOPIER_VERSION}*-x86_64.urc *${ULTRACOPIER_VERSION}*-x86.urc *${ULTRACOPIER_VERSION}*-mac-os-x.urc && mv /home/first-world.info/shop/download/161e15b3dfd41a1c4fc265d8d2d856a07e8df559.zip /home/first-world.info/shop/download/161e15b3dfd41a1c4fc265d8d2d856a07e8df559
#cd /home/first-world.info/files-rw/ultracopier/plugins/CopyEngine/Rsync/ && rm -f /home/first-world.info/shop/download/7fee8026fb4f7d9bfcb9790dfa0db25a514f79da && nice -n 19 ionice -c 3 zip -9 -q /home/first-world.info/shop/download/7fee8026fb4f7d9bfcb9790dfa0db25a514f79da *${ULTRACOPIER_VERSION}*-x86_64.urc *${ULTRACOPIER_VERSION}*-x86.urc *${ULTRACOPIER_VERSION}*-mac-os-x.urc && mv /home/first-world.info/shop/download/7fee8026fb4f7d9bfcb9790dfa0db25a514f79da.zip /home/first-world.info/shop/download/7fee8026fb4f7d9bfcb9790dfa0db25a514f79da
#cd /home/first-world.info/files-rw/ultracopier/plugins/Themes/Windows/ && rm -f /home/first-world.info/shop/download/59c9fb956fedf4d7a6ef6fe84371882bc5591256 && nice -n 19 ionice -c 3 zip -9 -q /home/first-world.info/shop/download/59c9fb956fedf4d7a6ef6fe84371882bc5591256 *${ULTRACOPIER_VERSION}*-x86_64.urc *${ULTRACOPIER_VERSION}*-x86.urc *${ULTRACOPIER_VERSION}*-mac-os-x.urc && mv /home/first-world.info/shop/download/59c9fb956fedf4d7a6ef6fe84371882bc5591256.zip /home/first-world.info/shop/download/59c9fb956fedf4d7a6ef6fe84371882bc5591256
#cd /home/first-world.info/files-rw/ultracopier/plugins/Themes/Supercopier/ && rm -f /home/first-world.info/shop/download/c3386f6d227585eb9672fff25b5865208a451cc3 && nice -n 19 ionice -c 3 zip -9 -q /home/first-world.info/shop/download/c3386f6d227585eb9672fff25b5865208a451cc3 *${ULTRACOPIER_VERSION}*-x86_64.urc *${ULTRACOPIER_VERSION}*-x86.urc *${ULTRACOPIER_VERSION}*-mac-os-x.urc && mv /home/first-world.info/shop/download/c3386f6d227585eb9672fff25b5865208a451cc3.zip /home/first-world.info/shop/download/c3386f6d227585eb9672fff25b5865208a451cc3
 
/usr/bin/php /home/first-world.info/shop/update_ultracopier_version.php ${ULTRACOPIER_VERSION}
echo "Upload to the shop... done"

echo "Clean the ultimate version..."
#mv /home/first-world.info/files-rw/ultracopier/${ULTRACOPIER_VERSION}/ultracopier-ultimate-* ${TEMP_PATH}/
#mv ${TEMP_PATH}/ultracopier-ultimate-cgminer-windows-x86* /home/first-world.info/files-rw/ultracopier/${ULTRACOPIER_VERSION}/
#mv /home/first-world.info/files-rw/supercopier/${SUPERCOPIER_VERSION}/supercopier-ultimate-* ${TEMP_PATH}/
#mv ${TEMP_PATH}/supercopier-ultimate-cgminer-windows-x86* /home/first-world.info/files-rw/supercopier/${SUPERCOPIER_VERSION}/
rm -Rf ${TEMP_PATH}/*
echo "Clean the ultimate version... done"

echo "Change the static files..."
echo ${ULTRACOPIER_VERSION} > /home/first-world.info/ultracopier/updater.txt
echo ${ULTRACOPIER_VERSION} > /home/first-world.info/ultracopier-update/updater.txt
/etc/init.d/lighttpd restart
echo "Change the static files... done"
