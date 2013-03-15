#!/bin/bash
./1-pre-send.sh

export TEMP_PATH="/home/ultracopier-temp/"
export WINEBASEPATH="/home/wine/"
export ULTRACOPIERSOURCESPATH="/root/ultracopier/sources/"
export BASE_PWD=`pwd`

export ULTRACOPIER_VERSION=`grep -F "ULTRACOPIER_VERSION" ${ULTRACOPIERSOURCESPATH}/Variable.h | grep -F "1.0" | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`

echo "Do the test folder..."
source sub-script/test.sh
cd ${BASE_PWD}
echo "Do the test folder... done"

./4-clean-all.sh

cp /home/ultracopier-temp/*.zip /home/first-world.info/files/temp/

