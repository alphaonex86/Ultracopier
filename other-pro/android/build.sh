#!/bin/sh
cd /home/user/src/
mkdir ~/build && cd ~/build
qmake -r ~/src/other-pro/ultracopier-little.pro ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libcrypto.so ANDROID_EXTRA_LIBS+=$ANDROID_DEV/lib/libssl.so
make -j32
make install INSTALL_ROOT=/home/user/build/dist/
/usr/lib/qt6/bin/androiddeployqt --input android-libultracopier.so-deployment-settings.json --output dist/ --android-platform 28 --deployment bundled --gradle --release
exit 0
cd /
ln -s opt/android-sdk/platforms
cd /home/user/src/
/opt/Qt/6.6.1/android_arm64_v8a/bin/qmake -platform android-clang -r ~/src/other-pro/ultracopier-little.pro CONFIG-=debug CONFIG+=release
/opt/android-sdk/ndk/25.1.8937393/prebuilt/linux-x86_64/bin/make -j32 aab
#make install INSTALL_ROOT=/home/user/build/dist/
#/usr/lib/qt6/bin/androiddeployqt --aab --input android-libultracopier.so-deployment-settings.json --output dist/ --android-platform 29 --deployment bundled --gradle --release
