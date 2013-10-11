#!/bin/bash
export TEMP_PATH="/home/ultracopier-temp/"
export WINEBASEPATH="/home/wine/"
export ULTRACOPIERSOURCESPATH="/root/ultracopier/sources/"
export BASE_PWD=`pwd`

export ULTRACOPIER_VERSION=`grep -F "ULTRACOPIER_VERSION" ${ULTRACOPIERSOURCESPATH}/Variable.h | grep -F "1.0" | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`

rm -Rf ${TEMP_PATH} > /dev/null 2>&1
rm -Rf ${ULTRACOPIERSOURCESPATH}/plugins-alternative/CopyEngine/Ultracopier/ > /dev/null 2>&1
mkdir -p ${TEMP_PATH}
find ../ -name "Thumbs.db" -exec rm {} \; >> /dev/null 2>&1
find ../ -name ".directory" -exec rm {} \; >> /dev/null 2>&1

echo "Do the test folder..."
source sub-script/test.sh
cd ${BASE_PWD}
echo "Do the test folder... done"

./4-clean-all.sh

cp /home/ultracopier-temp/*.zip /home/first-world.info/files/temp/

