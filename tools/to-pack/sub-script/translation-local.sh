#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
        exit;
fi

rm -Rf ${TEMP_PATH}

cd ${ULTRACOPIER_SOURCE}
lupdate ultracopier-core.pro > /dev/null 2>&1
lrelease -nounfinished -compress -removeidentical ultracopier-core.pro > /dev/null 2>&1
PWD_BASE2=`pwd`
echo "update the .ts file"
for project in `find plugins/ plugins-alternative/ -maxdepth 2 -type d`
do
        cd ${project}/
	for projectfile in `find ./ -name '*.pro' -type f`
	do
	        if [ -f ${projectfile} ]
	        then
	                lupdate ${projectfile} > /dev/null 2>&1
	                lrelease -nounfinished -compress -removeidentical ${projectfile} > /dev/null 2>&1
	        fi
	done
        cd ${PWD_BASE2}
done



