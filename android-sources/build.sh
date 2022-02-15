#!/bin/sh
cd /home/user/src/
mkdir ~/build && cd ~/build
qmake -r ~/src/other-pro/ultracopier-little.pro ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libcrypto.so ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libssl.so
make -j16
make install INSTALL_ROOT=/home/user/build/dist/
androiddeployqt --input android-libultracopier.so-deployment-settings.json --output dist/ --android-platform 28 --deployment bundled --gradle --release
exit 0
cd /
ln -s opt/android-sdk/platforms
cd /home/user/src/
qmake -platform android-clang -r ~/src/other-pro/ultracopier-little.pro CONFIG-=debug CONFIG+=release
make -j16 aab
#make install INSTALL_ROOT=/home/user/build/dist/
#androiddeployqt --aab --input android-libultracopier.so-deployment-settings.json --output dist/ --android-platform 29 --deployment bundled --gradle --release
