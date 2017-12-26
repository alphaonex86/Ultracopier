#!/bin/bash
export ULTRACOPIERSOURCESPATH="/root/ultracopier/sources/"
export ULTRACOPIER_VERSION=`grep -F "ULTRACOPIER_VERSION" ${ULTRACOPIERSOURCESPATH}/Variable.h | grep -F "1.4" | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`

./1-pre-send.sh
./2-compil-wine32.sh
#./2-compil-wine64.sh
./4-clean-all.sh
#mv /home/ultracopier-temp/ultracopier-*.* /home/first-world.info/files-rw/ultracopier/${ULTRACOPIER_VERSION}/
#mv /home/ultracopier-temp/supercopier-*.* /home/first-world.info/files-rw/supercopier/${ULTRACOPIER_VERSION}/
#rsync -avrt /home/ultracopier-temp/plugins/ /home/first-world.info/files-rw/ultracopier/plugins/
#rm -Rf /home/ultracopier-temp/plugins/
