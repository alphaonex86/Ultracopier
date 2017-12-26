
EAPI=4

LANGS="ar de el es fr hi id it ja nl no pl pt ru th tr zh"

inherit eutils qt5

DESCRIPTION="Advanced file copying tool"
HOMEPAGE="http://ultracopier.first-world.info/"
SRC_URI="http://files.first-world.info/${PN}/${PV}/ultracopier-src-${PV}.tar.xz -> ${P}.tar.xz"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64"
IUSE="debug"
S=${WORKDIR}/${P}/src/

RDEPEND="x11-libs/qt-core:5
	x11-libs/qt-gui:5"
DEPEND="${RDEPEND}"

DOCSDIR="${S}/"
DOCS="README"

src_prepare() {
	find -name "informations.xml" -exec sed -i -r "s/<architecture>.*<\/architecture>/<architecture>linux-x86_64-pc<\/architecture>/g" {} \; > /dev/null 2>&1
	find -name "informations.xml" -exec sed -i -r "s/<version>.*<\/version>/<version>${PV}<\/version>/g" {} \; > /dev/null 2>&1
	find -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_PORTABLE/\/\/#define ULTRACOPIER_VERSION_PORTABLE/g" {} \; > /dev/null 2>&1
	find -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_PORTABLEAPPS/\/\/#define ULTRACOPIER_VERSION_PORTABLEAPPS/g" {} \; > /dev/null 2>&1
	if use debug ; then
	then
		find -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_DEBUG/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
		find -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
		find -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
	else
		find -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_DEBUG/\/\/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1
		find -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG/\/\/#define ULTRACOPIER_PLUGIN_DEBUG/g" {} \; > /dev/null 2>&1
		find -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1
	fi
	find -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/\/\/#define ULTRACOPIER_PLUGIN_ALL_IN_ONE/g" {} \; > /dev/null 2>&1
	find -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_VERSION_ULTIMATE/#define ULTRACOPIER_VERSION_ULTIMATE/g" {} \; > /dev/null 2>&1
	eqmake4 "${S}"/ultracopier-core.pro
	eqmake4 "${S}"/plugins/CopyEngine/Ultracopier/CopyEngine.pro
	eqmake4 "${S}"/plugins/Listener/catchcopy-v0002/listener.pro
	eqmake4 "${S}"/plugins/plugins/SessionLoader/KDE4/sessionLoader.pro
	eqmake4 "${S}"/plugins/Themes/Oxygen/interface.pro
}

src_compile() {
	lrelease -nounfinished -compress -removeidentical -silent ultracopier-core.pro > /dev/null 2>&1 || die "Error when release the qm file"
	if [ -f Makefile ] ; then
		emake
	fi
	cd "${S}"/plugins/CopyEngine/Ultracopier/
	if [ -f Makefile ] ; then
		emake
	fi
	cd "${S}"/plugins/Listener/catchcopy-v0002/
	if [ -f Makefile ] ; then
		emake
	fi
	cd "${S}"/plugins/plugins/SessionLoader/KDE4/
	if [ -f Makefile ] ; then
		emake
	fi
	cd "${S}"/plugins/Themes/Oxygen/
	if [ -f Makefile ] ; then
		emake
	fi
}

src_install() {
	dobin ultracopier
	newicon resources/ultracopier-128x128.png ultracopier.png
	domenu resources/ultracopier.desktop
	
	insinto /usr/share/Ultracopier/CopyEngine/Ultracopier/
	doins plugins/CopyEngine/Ultracopier/informations.xml
	doins plugins/CopyEngine/Ultracopier/libcopyEngine.so
	fperms 0755 /plugins/CopyEngine/Ultracopier/libcopyEngine.so

	insinto /usr/share/Ultracopier/Listener/catchcopy-v0002/
	doins plugins/Listener/catchcopy-v0002/informations.xml
	doins plugins/Listener/catchcopy-v0002/liblistener.so
	fperms 0755 /plugins/Listener/catchcopy-v0002/liblistener.so

	insinto /usr/share/Ultracopier/SessionLoader/KDE4/
	doins plugins/SessionLoader/KDE4/informations.xml
	doins plugins/SessionLoader/KDE4/libsessionLoader.so
	fperms 0755 /plugins/SessionLoader/KDE4/libsessionLoader.so

	insinto /usr/share/Ultracopier/Themes/Oxygen/
	doins plugins/Themes/Oxygen/informations.xml
	doins plugins/Themes/Oxygen/libinterface.so
	fperms 0755 /plugins/Themes/Oxygen/libinterface.so

	lrelease -nounfinished -compress -removeidentical -silent ultracopier-core.pro > /dev/null 2>&1 || die "Error when release the qm file"
	for project in `find plugins/ plugins-alternative/ -maxdepth 2 -type d`
	do
		if [ -f ${project}/*.pro ]
		then
			lrelease -nounfinished -compress -removeidentical -silent ${project}/*.pro > /dev/null 2>&1 || die "Error when release the qm file"
		fi
	done
	find -iname "*.ts" -exec rm {} \; > /dev/null 2>&1 || die "Error when remove the ts file"

	# Install translations
	for Z in ${LANGS}; do
		if use linguas_${Z} ; then
			insinto /usr/share/Ultracopier/Languages/${Z}/
			doins -r plugins/Languages/${Z}/
			insinto /usr/share/Ultracopier/CopyEngine/Ultracopier/Languages/${Z}/
			doins -r plugins/CopyEngine/Ultracopier/Languages/${Z}/
			insinto /usr/share/Ultracopier/Themes/Oxygen/Languages/${Z}/
			doins -r plugins/Themes/Oxygen/Languages/${Z}/
		fi
	done
}
