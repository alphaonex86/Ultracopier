#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

cd ${TEMP_PATH}/

if [ ! -d ${TEMP_PATH}/doc/Ultracopier ]
then
	echo "Making Ultracopier doc..."
	cd ${BASE_PWD}/../doc/
	rm -Rf ${TEMP_PATH}/doc/tmp/
	mkdir -p ${TEMP_PATH}/doc/Ultracopier
	mkdir -p ${TEMP_PATH}/doc/tmp
	cp Doxyfile Doxyfile-tmp
	sed -i "s/_PROJECT_NUMBER_/${ULTRACOPIER_VERSION}/g" Doxyfile-tmp
	TEMP_PATH_DOXYGEN=`echo ${TEMP_PATH} | sed "s/\\//\\\\\\\\\\//g"`
	ULTRACOPIERSOURCESPATH_DOXYGEN=`echo ${ULTRACOPIERSOURCESPATH_DOXYGEN} | sed "s/\\//\\\\\\\\\\//g"`
	sed -i "s/TEMP_PATH/${TEMP_PATH_DOXYGEN}/g" Doxyfile-tmp
	sed -i "s/ULTRACOPIERSOURCESPATH/${ULTRACOPIERSOURCESPATH_DOXYGEN}/g" Doxyfile-tmp
	doxygen Doxyfile-tmp > /dev/null 2>&1
	rm Doxyfile-tmp
	rsync -art --delete ${TEMP_PATH}/doc/tmp/ ${TEMP_PATH}/doc/Ultracopier/${ULTRACOPIER_VERSION}/
	rm -Rf ${TEMP_PATH}/doc/tmp/
	echo "Making Ultracopier doc... done"
fi

if [ -d ${TEMP_PATH}/ultracopier-src/plugins/ ]
then
	echo "Making Ultracopier plugins doc..."
	cd ${TEMP_PATH}/ultracopier-src/plugins/
	for plugins_cat in `ls -1`
	do
		if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
		then
			cd ${TEMP_PATH}/ultracopier-src/plugins/${plugins_cat}/
			for plugins_name in `ls -1`
			do
				if [ -d ${plugins_name} ] &&  [ -f ${plugins_name}/informations.xml ]
				then
					cd ${TEMP_PATH}/ultracopier-src/plugins/${plugins_cat}/${plugins_name}/
					rm -Rf ${TEMP_PATH}/doc/${plugins_cat}/${plugins_name}/
					mkdir -p ${TEMP_PATH}/doc/${plugins_cat}/${plugins_name}/
					echo "Making Ultracopier plugins doc... for ${plugins_cat}/${plugins_name}"
					ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`

					cp ${BASE_PWD}/../doc/Doxyfile-plugin-template ${TEMP_PATH}/ultracopier-src/plugins/${plugins_cat}/${plugins_name}/Doxyfile
					cp ${BASE_PWD}/../doc/footer.html ${TEMP_PATH}/ultracopier-src/plugins/${plugins_cat}/${plugins_name}/footer.html

					ULTRACOPIER_PLUGIN_TITLE=`grep -F "<title" informations.xml | grep -F "lang=\"en\"" | sed -r "s/^.*<!\[CDATA\[(.*)\]\]>.*$//g"`
					
					sed -i "s/_PROJECT_BRIEF_/${ULTRACOPIER_PLUGIN_TITLE}/g" Doxyfile
					sed -i "s/_PROJECT_NUMBER_/${ULTRACOPIER_PLUGIN_VERSION}/g" Doxyfile
					sed -i "s/_ULTRACOPIER_PLUGIN_NAME_/${plugins_cat} - ${plugins_name}/g" Doxyfile
				        TEMP_PATH_DOXYGEN=`echo ${TEMP_PATH} | sed "s/\\//\\\\\\\\\\//g"`
				        ULTRACOPIERSOURCESPATH_DOXYGEN=`echo ${ULTRACOPIERSOURCESPATH_DOXYGEN} | sed "s/\\//\\\\\\\\\\//g"`
				        sed -i "s/TEMP_PATH/${TEMP_PATH_DOXYGEN}/g" Doxyfile
				        sed -i "s/ULTRACOPIERSOURCESPATH/${ULTRACOPIERSOURCESPATH_DOXYGEN}/g" Doxyfile

					doxygen Doxyfile > /dev/null 2>&1
					rm -f Doxyfile footer.html
					rsync -art ${TEMP_PATH}/doc/tmp/html/ ${TEMP_PATH}/doc/${plugins_cat}/${plugins_name}/${ULTRACOPIER_PLUGIN_VERSION}/
					cd ${TEMP_PATH}/ultracopier-src/plugins/${plugins_cat}/
				fi
				cd ${TEMP_PATH}/ultracopier-src/plugins/${plugins_cat}/
			done
			cd ${TEMP_PATH}/ultracopier-src/plugins/
		fi
	done
	echo "Making Ultracopier plugins doc... done"
fi

if [ -d ${TEMP_PATH}/ultracopier-src/plugins/ ]
then
	echo "Making Ultracopier plugins alternative doc..."
	cd ${TEMP_PATH}/ultracopier-src/plugins-alternative/
	for plugins_cat in `ls -1`
	do
		if [ -d ${plugins_cat} ] && [ "${plugins_cat}" != "Languages" ]
		then
			cd ${TEMP_PATH}/ultracopier-src/plugins-alternative/${plugins_cat}/
			for plugins_name in `ls -1`
			do
				if [ -d ${plugins_name} ] &&  [ -f ${plugins_name}/informations.xml ]
				then
					cd ${TEMP_PATH}/ultracopier-src/plugins-alternative/${plugins_cat}/${plugins_name}/
					rm -Rf ${TEMP_PATH}/doc/${plugins_cat}/${plugins_name}/
					mkdir -p ${TEMP_PATH}/doc/${plugins_cat}/${plugins_name}/
					echo "Making Ultracopier plugins doc... for ${plugins_cat}/${plugins_name}"
					ULTRACOPIER_PLUGIN_VERSION=`grep -F "<version>" informations.xml | sed -r "s/^.*([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$/\1/g"`

					cp ${BASE_PWD}/../doc/Doxyfile-plugin-template ${TEMP_PATH}/ultracopier-src/plugins-alternative/${plugins_cat}/${plugins_name}/Doxyfile
					cp ${BASE_PWD}/../doc/footer.html ${TEMP_PATH}/ultracopier-src/plugins-alternative/${plugins_cat}/${plugins_name}/footer.html

					ULTRACOPIER_PLUGIN_TITLE=`grep -F "<title" informations.xml | grep -F "lang=\"en\"" | sed -r "s/^.*<!\[CDATA\[(.*)\]\]>.*$//g"`

					sed -i "s/_PROJECT_BRIEF_/${ULTRACOPIER_PLUGIN_TITLE}/g" Doxyfile
					sed -i "s/_PROJECT_NUMBER_/${ULTRACOPIER_PLUGIN_VERSION}/g" Doxyfile
					sed -i "s/_ULTRACOPIER_PLUGIN_NAME_/${plugins_cat} - ${plugins_name}/g" Doxyfile
				        TEMP_PATH_DOXYGEN=`echo ${TEMP_PATH} | sed "s/\\//\\\\\\\\\\//g"`
				        ULTRACOPIERSOURCESPATH_DOXYGEN=`echo ${ULTRACOPIERSOURCESPATH_DOXYGEN} | sed "s/\\//\\\\\\\\\\//g"`
				        sed -i "s/TEMP_PATH/${TEMP_PATH_DOXYGEN}/g" Doxyfile
					sed -i "s/ULTRACOPIERSOURCESPATH/${ULTRACOPIERSOURCESPATH_DOXYGEN}/g" Doxyfile

					doxygen Doxyfile > /dev/null 2>&1
					rm -f Doxyfile footer.html
					rsync -art ${TEMP_PATH}/doc/tmp/html/ ${TEMP_PATH}/doc/${plugins_cat}/${plugins_name}/${ULTRACOPIER_PLUGIN_VERSION}/
					cd ${TEMP_PATH}/ultracopier-src/plugins-alternative/${plugins_cat}/
				fi
			done
			cd ${TEMP_PATH}/ultracopier-src/plugins-alternative/
		fi
	done
	echo "Making Ultracopier plugins alternative doc... done"
fi

rm -Rf ${TEMP_PATH}/doc/tmp/
