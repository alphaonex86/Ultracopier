#!/bin/bash

export TEMP_PATH="/mnt/world/ultracopier-temp/"
export ULTRACOPIER_SOURCE="/home/user/Desktop/ultracopier/sources/"
export BASE_PWD=`pwd`

cd ${BASE_PWD}

export ULTRACOPIER_VERSION=`grep -F "ULTRACOPIER_VERSION" ${ULTRACOPIER_SOURCE}/Variable.h | grep -F "0.4" | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
echo ${ULTRACOPIER_VERSION}

echo "Upload version..."
source sub-script/upload-local.sh
cd ${BASE_PWD}
echo "Upload version... done"



 
