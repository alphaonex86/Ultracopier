#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi
if [ "${ULTRACOPIER_VERSION}" = "" ]
then
        exit;
fi
SUPERCOPIER_VERSION=`echo "${ULTRACOPIER_VERSION}" | sed -r "s/1.0.([0-9]+\\.[0-9]+)/4.0.\1/g"`

cd ${TEMP_PATH}/

echo "Send mail..."
/usr/bin/php /home/first-world.info/ultracopier-shop/sendmail_ultracopier_version.php ${ULTRACOPIER_VERSION}
echo "Send mail... done"
