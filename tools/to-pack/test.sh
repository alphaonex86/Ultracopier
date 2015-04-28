#!/bin/bash
export TEMP_PATH="/home/ultracopier-temp/"
export WINEBASEPATH="/home/wine/"
export ULTRACOPIERSOURCESPATH="/root/ultracopier/sources/"
export BASE_PWD=`pwd`

export ULTRACOPIER_VERSION=`grep -F "ULTRACOPIER_VERSION" ${ULTRACOPIERSOURCESPATH}/Variable.h | grep -F "1.2" | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`
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

rm /home/first-world.info/ultracopier/temp/*
cp /home/ultracopier-temp/*.zip /home/first-world.info/ultracopier/temp/
cp /home/ultracopier-temp/*-setup.exe /home/first-world.info/ultracopier/temp/

