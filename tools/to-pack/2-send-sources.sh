#!/bin/bash

cd ../../
find ./ -name "Thumbs.db" -exec rm {} \; >> /dev/null 2>&1
find ./ -name ".directory" -exec rm {} \; >> /dev/null 2>&1

echo "Send sources..."
/usr/bin/rsync -avrtz --compress-level=9 --rsh='ssh -p54973' --delete --partial --progress /home/user/Desktop/ultracopier/sources/ root@ssh.first-world.info:/root/ultracopier/sources/ --exclude='*build*' --exclude='*Qt_5*' --exclude='*qt5*' --exclude='*.pro.user' --exclude='*.qm'
echo "Send sources... done"



