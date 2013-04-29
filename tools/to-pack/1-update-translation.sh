#!/bin/bash

export TEMP_PATH="/mnt/world/ultracopier-temp/"
export ULTRACOPIER_SOURCE="/home/user/Desktop/ultracopier/sources/"
export BASE_PWD=`pwd`

cd ${BASE_PWD}

export ULTRACOPIER_VERSION=`grep -F "ULTRACOPIER_VERSION" ${ULTRACOPIER_SOURCE}/Variable.h | grep -F "1.0" | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
echo ${ULTRACOPIER_VERSION}

echo "Update translation..."
source sub-script/translation-local.sh
cd ${BASE_PWD}
echo "Update translation... done"



