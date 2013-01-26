#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi
if [ "${ULTRACOPIER_VERSION}" = "" ]
then
        exit;
fi

cd ${TEMP_PATH}/

MYSQL_USER="ultracopier-shop"
MYSQL_DB="other_ultracopier-shop"
MYSQL_PASS="FzdFbpvp4Uq6ZMjz"

echo "Move some elements..."
if [ -d ${TEMP_PATH}/doc/ ]
then
	rsync -art ${TEMP_PATH}/doc/ /home/first-world.info/doc-ultracopier/
	if [ $? -ne 0 ]
	then
		echo 'rsync failed'
	        exit;
	fi
	rm -Rf ${TEMP_PATH}/doc/
fi
if [ -d ${TEMP_PATH}/plugins/ ]
then
	rsync -art ${TEMP_PATH}/plugins/ /home/first-world.info/files/ultracopier/plugins/
	if [ $? -ne 0 ]
	then
	        echo 'rsync failed'
	        exit;
	fi
	rm -Rf ${TEMP_PATH}/plugins/
fi
mkdir -p /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/
mv ${TEMP_PATH}/*.tar.xz /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/
mv ${TEMP_PATH}/*.zip /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/
mv ${TEMP_PATH}/*-setup.exe /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/
mv ${TEMP_PATH}/*.tar.bz2 /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/
echo "Move some elements... done"

echo "Finalise some elements..."
chown lighttpd.lighttpd -Rf /home/first-world.info/doc-ultracopier/
chown lighttpd.lighttpd -Rf /home/first-world.info/files/ultracopier/plugins/
echo "Finalise some elements... done"

echo "Upload to the shop..."
#cd /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/ && rm -f /home/first-world.info/ultracopier-shop/download/1190342cc5284204ab9a04ac4f66588ce9d2d350 && nice -n 19 ionice -c 3 zip -9 -q /home/first-world.info/ultracopier-shop/download/1190342cc5284204ab9a04ac4f66588ce9d2d350 ultracopier-ultimate-*-x86-${ULTRACOPIER_VERSION}-setup.exe && mv /home/first-world.info/ultracopier-shop/download/1190342cc5284204ab9a04ac4f66588ce9d2d350.zip /home/first-world.info/ultracopier-shop/download/1190342cc5284204ab9a04ac4f66588ce9d2d350
cd /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/ && rm -f /home/first-world.info/ultracopier-shop/download/d506a059870620e2362eeca3f855b886243d2d79 && nice -n 19 ionice -c 3 zip -9 -q /home/first-world.info/ultracopier-shop/download/d506a059870620e2362eeca3f855b886243d2d79 ultracopier-ultimate-*-x86_64-${ULTRACOPIER_VERSION}-setup.exe && mv /home/first-world.info/ultracopier-shop/download/d506a059870620e2362eeca3f855b886243d2d79.zip /home/first-world.info/ultracopier-shop/download/d506a059870620e2362eeca3f855b886243d2d79
cp /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/ultracopier-ultimate-linux-x86_64-pc-${ULTRACOPIER_VERSION}.tar.xz /home/first-world.info/ultracopier-shop/download/68253ed54831d6101ca68a32838d874357123a0a
cp /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/ultracopier-ultimate-mac-os-x-${ULTRACOPIER_VERSION}.dmg /home/first-world.info/ultracopier-shop/download/7c0479c7d5569db6d1b20f3b3fd73381c39a9838

cd /home/first-world.info/files/ultracopier/plugins/Themes/Teracopy/ && nice -n 19 ionice -c 3 tar cpf - *x86_64.urc *x86.urc *mac-os-x.urc *linux-x86_64-pc.urc | nice -n 19 ionice -c 3 xz -z -9 -e > /home/first-world.info/ultracopier-shop/download/985eb33aaa387ac0f28a67278acb23dcf8b8ac20
cd /home/first-world.info/files/ultracopier/plugins/CopyEngine/Rsync/ && nice -n 19 ionice -c 3 tar cpf - *x86_64.urc *x86.urc *mac-os-x.urc *linux-x86_64-pc.urc | nice -n 19 ionice -c 3 xz -z -9 -e > /home/first-world.info/ultracopier-shop/download/40fc6d58e5afd283b9169f91c5719c76e8d319d2
cd /home/first-world.info/files/ultracopier/plugins/Themes/Windows/ && nice -n 19 ionice -c 3 tar cpf - *x86_64.urc *x86.urc *mac-os-x.urc *linux-x86_64-pc.urc | nice -n 19 ionice -c 3 xz -z -9 -e > /home/first-world.info/ultracopier-shop/download/9482a21d6a3e03fa5612f6a2b75cd109fd8bdec4

#echo "UPDATE product_download SET display_filename='ultracopier-ultimate-windows-x86-all-${ULTRACOPIER_VERSION}.zip' WHERE id_product_download=1;" | mysql -u ${MYSQL_USER} -p${MYSQL_PASS} ${MYSQL_DB}
echo "UPDATE product_download SET display_filename='ultracopier-ultimate-windows-x86_64-all-${ULTRACOPIER_VERSION}.zip' WHERE id_product_download=2;" | mysql -u ${MYSQL_USER} -p${MYSQL_PASS} ${MYSQL_DB}
echo "UPDATE product_download SET display_filename='ultracopier-ultimate-mac-${ULTRACOPIER_VERSION}.dmg' WHERE id_product_download=3;" | mysql -u ${MYSQL_USER} -p${MYSQL_PASS} ${MYSQL_DB}
echo "UPDATE product_download SET display_filename='Themes-Teracopy-${ULTRACOPIER_VERSION}.tar.xz' WHERE id_product_download=7;" | mysql -u ${MYSQL_USER} -p${MYSQL_PASS} ${MYSQL_DB}
echo "UPDATE product_download SET display_filename='CopyEngine-Rsync-${ULTRACOPIER_VERSION}.tar.xz' WHERE id_product_download=8;" | mysql -u ${MYSQL_USER} -p${MYSQL_PASS} ${MYSQL_DB}
echo "UPDATE product_download SET display_filename='Themes-Windows-${ULTRACOPIER_VERSION}.tar.xz' WHERE id_product_download=15;" | mysql -u ${MYSQL_USER} -p${MYSQL_PASS} ${MYSQL_DB}
/usr/bin/php /home/first-world.info/ultracopier-shop/update_ultracopier_version.php ${ULTRACOPIER_VERSION}
echo "Upload to the shop... done"

#echo "Clean the ultimate version..."
#rm -f /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/ultracopier-ultimate*
#echo "Clean the ultimate version... done"

