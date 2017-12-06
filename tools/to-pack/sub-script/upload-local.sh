#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

mkdir -p ${TEMP_PATH}
cd ${TEMP_PATH}/

rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --partial --progress /mnt/world/ultracopier-temp/ultracopier-*.dmg root@ssh.first-world.info:/home/first-world.info/files-rw/ultracopier/${ULTRACOPIER_VERSION}/ --timeout=120
RETURNA=$?
while [ ${RETURNA} -ne 0 ] && [ ${RETURNA} -ne 20 ] && [ ${RETURNA} -ne 255 ]
do
	rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --partial --progress /mnt/world/ultracopier-temp/ultracopier-*.dmg root@ssh.first-world.info:/home/first-world.info/files-rw/ultracopier/${ULTRACOPIER_VERSION}/ --timeout=120
	RETURNA=$?
	echo ${RETURNA}
done


rsync -avrtzu --compress-level=9 --rsh='ssh -p54973' --partial --progress /mnt/world/ultracopier-temp/plugins/ root@ssh.first-world.info:/home/first-world.info/files-rw/ultracopier/plugins/ --timeout=120
RETURNB=$?
while [ ${RETURNB} -ne 0 ] && [ ${RETURNB} -ne 20 ] && [ ${RETURNB} -ne 255 ]
do
        rsync -avrtzu --compress-level=9 --rsh='ssh -p54973' --partial --progress /mnt/world/ultracopier-temp/plugins/ root@ssh.first-world.info:/home/first-world.info/files-rw/ultracopier/plugins/ --timeout=120
        RETURNB=$?
        echo ${RETURNB}
done

#if [ "${RETURNA}" -eq 0 ] && [ "${RETURNB}" -eq 0 ]
#then
#	rm -Rf /mnt/world/ultracopier-temp/*.dmg /mnt/world/ultracopier-temp/*.tar.xz /mnt/world/ultracopier-temp/plugins/
#fi
