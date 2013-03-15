#!/bin/bash

export TEMP_PATH="/home/ultracopier-temp/"
export WINEBASEPATH="/home/wine/"
export ULTRACOPIERSOURCESPATH="/root/ultracopier/sources/"
export BASE_PWD=`pwd`

rm -Rf ${TEMP_PATH} > /dev/null 2>&1
mkdir -p ${TEMP_PATH}
cd ../
find ./ -name "Thumbs.db" -exec rm {} \; >> /dev/null 2>&1
find ./ -name ".directory" -exec rm {} \; >> /dev/null 2>&1

cd ${BASE_PWD}

export ULTRACOPIER_VERSION=`grep -F "ULTRACOPIER_VERSION" ${ULTRACOPIERSOURCESPATH}/Variable.h | grep -F "1.0" | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
echo ${ULTRACOPIER_VERSION}

echo "Update the translation..."
source sub-script/translation.sh
cd ${BASE_PWD}
echo "Update the translation... done"

echo "Assemble source version..."
source sub-script/assemble-source-version.sh
cd ${BASE_PWD}
echo "Assemble source version... done"

echo "Assemble doc version..."
source sub-script/doc.sh
cd ${BASE_PWD}
echo "Assemble doc version... done"



