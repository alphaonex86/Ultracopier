#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

TARGET="ultracopier-translation-${ULTRACOPIER_VERSION}"
rm -Rf ${TEMP_PATH}/${TARGET}/
mkdir -p ${TEMP_PATH}/${TARGET}/
cd ${ULTRACOPIERSOURCESPATH}
lupdate -no-obsolete ultracopier-core.pro > /dev/null 2>&1
lrelease -nounfinished -compress -removeidentical ultracopier-core.pro > /dev/null 2>&1
PWD_BASE2=`pwd`
echo "update the .ts file"
for project in `find plugins/ plugins-alternative/ -maxdepth 2 -type d`
do
	cd ${project}/
	if [ -f *.pro ]
	then
		lupdate -no-obsolete *.pro > /dev/null 2>&1
		lrelease -nounfinished -compress -removeidentical *.pro > /dev/null 2>&1
	fi
	cd ${PWD_BASE2}
done
echo "copy the .ts file"
for languages in `find ./ -name Languages -type d`
do
	mkdir -p ${TEMP_PATH}/${TARGET}/${languages}
	cp -aRf ${languages} ${TEMP_PATH}/${TARGET}/${languages}/../
done
cd ../to-pack/

rm -Rf ${TEMP_PATH}/${TARGET}/*/*/Rsync > /dev/null 2>&1
rm -Rf ${TEMP_PATH}/${TARGET}/plugins-alternative/CopyEngine > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -name "*.qm" -exec rm {} \; > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -name "Test" -type d -exec rm -Rf {} \; > /dev/null 2>&1
mv ${TEMP_PATH}/${TARGET}/resources/Languages/en/ ${TEMP_PATH}/${TARGET}/plugins/Languages/en/
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
find ${TEMP_PATH}/${TARGET}/ -type d -empty -delete > /dev/null 2>&1
rm -Rf ${TEMP_PATH}/${TARGET}/resources/

cd ${TEMP_PATH}/
tar cjpf ${TARGET}.tar.bz2 ${TARGET}/
if [ ! -e ${TARGET}.tar.bz2 ]; then
	echo "${TARGET}.tar.bz2 not exists!";
	exit;
fi
rm -Rf ${TARGET}/
