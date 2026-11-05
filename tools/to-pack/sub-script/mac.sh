#!/bin/bash

# macOS build, run entirely on this Linux host from the local osxcross prefix
# (history: qemu mac VM 192.168.158.34 -> osxcross LXC root@2803:1920::2:ff08
# -> local prefix, no remote host anymore). Same flow as CatchChallenger's
# test/testingcompilationmac.py. The prefix holds:
#   - osxcross (clang, target darwin20.4 = macOS 11+) + <arch>-apple-darwin20.4-cmake wrappers
#   - Qt 6.5.3 for macOS (universal x86_64+arm64 frameworks/plugins)
#   - Qt 6.5.3 host-native (Linux ELF) moc/uic/rcc; this workstation's own Qt 6.11
#     tools do NOT work: their cmake configs call _qt_internal_should_include_targets(),
#     undefined in 6.5.3's cmake modules -> configure error
#   - rcodesign (cargo install apple-codesign --locked --root <prefix>): ad-hoc signing,
#     install_name_tool breaks the arm64 signatures and Apple Silicon refuses to run them
#   - libdmg-hfsplus `dmg` + genisoimage (Debian build: the host's cdrtools mkisofs
#     lacks the siconv charset tables, `-hfs` dies on 'Unknown HFS charset cp10000')
# ninja, lld, ccache and python3 come from this host.
# Universal binary: build once per arch, lipo the two executables into one
# .app; the Qt frameworks are already fat so one deploy pass covers both.

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi

mkdir -p ${TEMP_PATH}
cd ${TEMP_PATH}/

OSXPREFIX="/mnt/data/perso/progs/catchchallenger-osxcross"
OSXQT="${OSXPREFIX}/qt6-macos/6.5.3/macos"
OSXHOSTQTCMAKE="${OSXPREFIX}/qt6-host/6.5.3/gcc_64/lib/cmake"
OSXCROSSBIN="${OSXPREFIX}/osxcross/target/bin"
OSXISOTOOL="${OSXPREFIX}/bin/genisoimage"
OSXDMGBIN="${OSXPREFIX}/libdmg-hfsplus/build/dmg/dmg"
OSXCODESIGN="${OSXPREFIX}/bin/rcodesign"
ARCHS="x86_64 arm64"
LIPOTRIPLE="x86_64-apple-darwin20.4"
WORKDIR="${TEMP_PATH}/mac-build"
SRCDIR="${WORKDIR}/sources"
APPNAME="Ultracopier.app"
EXENAME="Ultracopier"

for TOOL in ${OSXCROSSBIN}/${LIPOTRIPLE}-cmake ${OSXISOTOOL} ${OSXDMGBIN} ${OSXCODESIGN}
do
	if [ ! -x ${TOOL} ]
	then
		echo "osxcross toolchain incomplete: ${TOOL} missing"
		exit
	fi
done
if [ ! -d ${OSXQT} ] || [ ! -d ${OSXHOSTQTCMAKE} ]
then
	echo "osxcross toolchain incomplete: Qt 6.5.3 (macos and/or host) missing under ${OSXPREFIX}"
	exit
fi

# MACOSX_DEPLOYMENT_TARGET=11.0; without it clang's availability checks reject
# std::filesystem in the Qt headers (needs 10.15+). CCACHE_DIR is pinned inside
# the prefix so the mac objects never mix with this workstation's cache.
source ${OSXPREFIX}/env.sh > /dev/null 2>&1
export PATH=${OSXCROSSBIN}:${PATH}
export CCACHE_DIR=${OSXPREFIX}/ccache

FINAL_ARCHIVE="ultracopier-mac-os-x-${ULTRACOPIER_VERSION}.dmg"
if [ ! -e ${FINAL_ARCHIVE} ]
then
	echo "Making Mac dmg: ${FINAL_ARCHIVE} ..."

	echo "try rsync"
	# Source only -- never copy in-source build artifacts (stale *.o, or moc_*/qrc_*/ui_* from
	# a different Qt); they get reused by mtime and break the build (same as android.sh).
	mkdir -p ${SRCDIR}
	rsync -art --delete --delete-excluded ${ULTRACOPIER_SOURCE}/ ${SRCDIR}/ --exclude=/.git/ --exclude=/build/ --exclude=/build-*/ --exclude=/test/ --exclude=/tools/ --exclude='*.o' --exclude='*.a' --exclude='*.so' --exclude='*.dylib' --exclude='*.exe' --exclude='*.dll' --exclude='moc_*.cpp' --exclude='moc_*.h' --exclude='qrc_*.cpp' --exclude='ui_*.h' --exclude='.qmake.stash' --exclude='*.pro.user' --exclude='CMakeLists.txt.user' --exclude='*.app'
	if [ $? -ne 0 ]
	then
		echo "rsync of the sources failed"
		exit
	fi

	# Stamp informations.xml (compiled into the binary via static-plugins.qrc) on the
	# copy only; the original source tree stays untouched. Re-done on every run
	# because the rsync above re-syncs the pristine files.
	find ${SRCDIR} -name informations.xml -exec sed -i -r -e 's|<architecture>.*</architecture>|<architecture>mac-os-x</architecture>|g' -e "s|<version>.*</version>|<version>${ULTRACOPIER_VERSION}</version>|g" -e "s|<pubDate>.*</pubDate>|<pubDate>`date +%s`</pubDate>|g" {} \;

	# Configure + build each arch. The shipping defines (NOAUDIO, ALL_IN_ONE(_DIRECT),
	# NODEBUG) are hardcoded in CMakeLists.txt, so no Variable.h editing is needed.
	# Qt6_DIR/FIND_ROOT_PATH pinning: the osxcross toolchain sets
	# CMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY, restricting find_package() to the SDK,
	# and the host-native moc/uic/rcc must come from the prefix's Qt 6.5.3 host build
	# instead of the Mach-O ones in the mac Qt (same flags as testingcompilationmac.py).
	for ARCH in ${ARCHS}
	do
		TRIPLE="${ARCH}-apple-darwin20.4"
		BUILDDIR="${WORKDIR}/build-${ARCH}"
		echo "try cmake configure (${ARCH})"
		rm -rf ${BUILDDIR}
		mkdir -p ${BUILDDIR}
		${OSXCROSSBIN}/${TRIPLE}-cmake -S ${SRCDIR} -B ${BUILDDIR} -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH "-DCMAKE_FIND_ROOT_PATH=${OSXQT};${OSXHOSTQTCMAKE}" "-DCMAKE_PREFIX_PATH=${OSXQT};/usr" -DQt6_DIR=${OSXQT}/lib/cmake/Qt6 -DQt6CoreTools_DIR=${OSXHOSTQTCMAKE}/Qt6CoreTools -DQt6GuiTools_DIR=${OSXHOSTQTCMAKE}/Qt6GuiTools -DQt6WidgetsTools_DIR=${OSXHOSTQTCMAKE}/Qt6WidgetsTools -DQt6DBusTools_DIR=${OSXHOSTQTCMAKE}/Qt6DBusTools -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache "-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld"
		if [ $? -ne 0 ]
		then
			echo "cmake configure failed (${ARCH})"
			exit
		fi
		echo "try build (${ARCH})"
		${OSXCROSSBIN}/${TRIPLE}-cmake --build ${BUILDDIR} -j$(nproc)
		if [ $? -ne 0 ]
		then
			echo "build failed (${ARCH})"
			exit
		fi
	done

	echo "try lipo (universal .app)"
	APPU="${WORKDIR}/${APPNAME}"
	rm -rf ${APPU}
	cp -a ${WORKDIR}/build-x86_64/${APPNAME} ${APPU} && \
	${OSXCROSSBIN}/${LIPOTRIPLE}-lipo -create ${WORKDIR}/build-x86_64/${APPNAME}/Contents/MacOS/${EXENAME} ${WORKDIR}/build-arm64/${APPNAME}/Contents/MacOS/${EXENAME} -output ${APPU}/Contents/MacOS/${EXENAME} && \
	${OSXCROSSBIN}/${LIPOTRIPLE}-lipo -info ${APPU}/Contents/MacOS/${EXENAME}
	if [ $? -ne 0 ]
	then
		echo "lipo merge failed"
		exit
	fi

	# CMakeLists.txt only knows the 3.0 project version; stamp the full one.
	python3 -c "import plistlib,sys;f=sys.argv[1];p=plistlib.load(open(f,'rb'));p['CFBundleVersion']='${ULTRACOPIER_VERSION}';p['CFBundleShortVersionString']='${ULTRACOPIER_VERSION}';plistlib.dump(p,open(f,'wb'))" ${APPU}/Contents/Info.plist
	if [ $? -ne 0 ]
	then
		echo "Info.plist stamping failed"
		exit
	fi

	# Qt's macdeployqt is a Mach-O binary that can't run on Linux; the
	# Linux-native equivalent bundles the Qt frameworks + plugins into the .app and
	# rewrites the install names to @executable_path/../Frameworks/, ad-hoc signs it and
	# fails if anything in the bundle would not load on a real Mac.
	echo "try macdeployqt_linux"
	python3 ${BASE_PWD}/sub-script/macdeployqt_linux.py --app ${APPU} --qt ${OSXQT} --osxcross-bin ${OSXCROSSBIN} --triple ${LIPOTRIPLE} --rcodesign ${OSXCODESIGN}
	if [ $? -ne 0 ]
	then
		echo "macdeployqt_linux.py failed"
		exit
	fi

	# No hdiutil on Linux: genisoimage makes an HFS hybrid image of a staging dir
	# holding the .app, then libdmg-hfsplus converts it to a compressed .dmg.
	echo "try dmg"
	STAGE=$(mktemp -d ${WORKDIR}/dmg-stage.XXXXXX)
	ISO=$(mktemp ${WORKDIR}/dmg.XXXXXX.iso)
	cp -a ${APPU} ${STAGE}/ && \
	${OSXISOTOOL} -quiet -V Ultracopier -no-pad -r -hfs -o ${ISO} ${STAGE} && \
	${OSXDMGBIN} ${ISO} ${WORKDIR}/ultracopier.dmg > /dev/null
	if [ $? -ne 0 ]
	then
		echo "dmg packaging failed"
		rm -rf ${STAGE} ${ISO}
		exit
	fi
	rm -rf ${STAGE} ${ISO}

	cp ${WORKDIR}/ultracopier.dmg ${TEMP_PATH}/${FINAL_ARCHIVE}
	if [ ! -e ${FINAL_ARCHIVE} ]; then
		echo "${FINAL_ARCHIVE} not exists!";
		exit;
	fi
	echo "Making Mac dmg... done"
else
	echo "Archive already exists: ${FINAL_ARCHIVE}"
fi
