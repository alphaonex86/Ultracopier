#!/bin/bash

export TEMP_PATH="/home/ultracopier-temp/"
export WINEBASEPATH="/home/wine/"
export ULTRACOPIERSOURCESPATH="/root/ultracopier/sources/"
export BASE_PWD=`pwd` 

export ULTRACOPIER_VERSION=`grep -F "ULTRACOPIER_VERSION" ${ULTRACOPIERSOURCESPATH}/Variable.h | grep -F "1.4" | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
function valid_ip()
{
    local  ip=$1
    local  stat=1
    if [[ $ip =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
        OIFS=$IFS
        IFS='.'
        ip=($ip)
        IFS=$OIFS
        [[ ${ip[0]} -le 255 && ${ip[1]} -le 255 \
            && ${ip[2]} -le 255 && ${ip[3]} -le 255 ]]
        stat=$?
    fi
    return $stat
}
if ! valid_ip ${ULTRACOPIER_VERSION}; then
        echo Wrong version: ${ULTRACOPIER_VERSION}
        exit
fi
echo Version: ${ULTRACOPIER_VERSION}

echo "Clean the temp folder..."
source sub-script/clean-all.sh
cd ${BASE_PWD}
echo "Clean the temp folder... done"


