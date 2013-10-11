#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

cd ${TEMP_PATH}/
for folder in `ls -1`
do
	if [ -d "${folder}" ] && [ "${folder}" != "plugins" ] && [ "${folder}" != "doc" ]
	then
		rm -Rf ${folder}/
	fi
done

