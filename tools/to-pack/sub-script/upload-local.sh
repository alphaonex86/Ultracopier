#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

mkdir -p ${TEMP_PATH}
cd ${TEMP_PATH}/

rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --partial --progress /mnt/world/ultracopier-temp/*.dmg /mnt/world/ultracopier-temp/*.tar.xz root@ssh.first-world.info:/home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/
RETURNA=$?
rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --partial --progress /mnt/world/ultracopier-temp/plugins/ root@ssh.first-world.info:/home/first-world.info/files/ultracopier/plugins/
RETURNB=$?

if [ "${RETURNA}" -eq 0 ] && [ "${RETURNB}" -eq 0 ]
then
	rm -Rf /mnt/world/ultracopier-temp/*.dmg /mnt/world/ultracopier-temp/*.tar.xz /mnt/world/ultracopier-temp/plugins/
fi

