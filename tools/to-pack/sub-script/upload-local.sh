#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

mkdir -p ${TEMP_PATH}
cd ${TEMP_PATH}/

SUPERCOPIER_VERSION=`echo "${ULTRACOPIER_VERSION}" | sed -r "s/1.0.([0-9]+\\.[0-9]+)/4.0.\1/g"`

rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --partial --progress /mnt/world/ultracopier-temp/ultracopier-*.dmg /mnt/world/ultracopier-temp/ultracopier-*.tar.xz root@ssh.first-world.info:/home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/ --timeout=120
RETURNA=$?
while [ ${RETURNA} -ne 0 ] && [ ${RETURNA} -ne 20 ] && [ ${RETURNA} -ne 255 ]
do
	rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --partial --progress /mnt/world/ultracopier-temp/ultracopier-*.dmg /mnt/world/ultracopier-temp/ultracopier-*.tar.xz root@ssh.first-world.info:/home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/ --timeout=120
	RETURNA=$?
	echo ${RETURNA}
done



rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --partial --progress /mnt/world/ultracopier-temp/supercopier-*.dmg /mnt/world/ultracopier-temp/supercopier-*.tar.xz root@ssh.first-world.info:/home/first-world.info/files/supercopier/${SUPERCOPIER_VERSION}/ --timeout=120
RETURNA=$?
while [ ${RETURNA} -ne 0 ] && [ ${RETURNA} -ne 20 ] && [ ${RETURNA} -ne 255 ]
do
	rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --partial --progress /mnt/world/ultracopier-temp/supercopier-*.dmg /mnt/world/ultracopier-temp/supercopier-*.tar.xz root@ssh.first-world.info:/home/first-world.info/files/supercopier/${SUPERCOPIER_VERSION}/ --timeout=120
	RETURNA=$?
	echo ${RETURNA}
done



rsync -avrtzu --compress-level=9 --rsh='ssh -p54973' --partial --progress /mnt/world/ultracopier-temp/plugins/ root@ssh.first-world.info:/home/first-world.info/files/ultracopier/plugins/ --timeout=120
RETURNB=$?
while [ ${RETURNB} -ne 0 ] && [ ${RETURNB} -ne 20 ] && [ ${RETURNB} -ne 255 ]
do
        rsync -avrtzu --compress-level=9 --rsh='ssh -p54973' --partial --progress /mnt/world/ultracopier-temp/plugins/ root@ssh.first-world.info:/home/first-world.info/files/ultracopier/plugins/ --timeout=120
        RETURNB=$?
        echo ${RETURNB}
done

#if [ "${RETURNA}" -eq 0 ] && [ "${RETURNB}" -eq 0 ]
#then
#	rm -Rf /mnt/world/ultracopier-temp/*.dmg /mnt/world/ultracopier-temp/*.tar.xz /mnt/world/ultracopier-temp/plugins/
#fi

