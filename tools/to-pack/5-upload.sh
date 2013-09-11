#!/bin/bash

export TEMP_PATH="/home/ultracopier-temp/"
export WINEBASEPATH="/home/wine/"
export ULTRACOPIERSOURCESPATH="/root/ultracopier/sources/"
export BASE_PWD=`pwd`

export ULTRACOPIER_VERSION=`grep -F "ULTRACOPIER_VERSION" ${ULTRACOPIERSOURCESPATH}/Variable.h | grep -F "1.0" | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`

echo "upload..."
source sub-script/upload.sh
cd ${BASE_PWD}
echo "upload... done"


