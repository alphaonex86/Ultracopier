
EAPI=4

LANGS="ar de el es fr hi id it ja nl no pl pt ru th tr zh"

inherit eutils qt4-r2

DESCRIPTION="Advanced file copying tool"
HOMEPAGE="http://ultracopier.first-world.info/"
SRC_URI="http://files.first-world.info/${PN}/${PV}/ultracopier-src-${PV}.tar.xz -> ${P}.tar.xz"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64"
IUSE="debug"
S=${WORKDIR}/${P}/src/

RDEPEND="x11-libs/qt-core:4
	x11-libs/qt-gui:4"
DEPEND="${RDEPEND}"

DOCSDIR="${S}/"
DOCS="README"

src_prepare() {
	find -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_VERSION_PORTABLE/\/\/#define ULTRACOPIER_VERSION_PORTABLE/g" {} \; > /dev/null 2>&1 || die "Error when prepare the application"
	if use debug ; then
		find -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_DEBUG/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1 || die "Error when prepare the application"
		find -name "Variable.h" -exec sed -i "s/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1 || die "Error when prepare the application"
	else
		find -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_DEBUG/\/\/#define ULTRACOPIER_DEBUG/g" {} \; > /dev/null 2>&1 || die "Error when prepare the application"
		find -name "Variable.h" -exec sed -i "s/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/\/\/#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW/g" {} \; > /dev/null 2>&1 || die "Error when prepare the application"
	fi
	eqmake4 "${S}"/ultracopier-core.pro
	eqmake4 "${S}"/plugins/CopyEngine/Ultracopier/copyEngine.pro
	eqmake4 "${S}"/plugins/Listener/catchcopy-v0002/listener.pro
	eqmake4 "${S}"/plugins/plugins/SessionLoader/KDE4/sessionLoader.pro
	eqmake4 "${S}"/plugins/Themes/Oxygen/interface.pro
}

src_compile() {
	if [ -f Makefile ] ; then
		emake
	fi
	cd "${S}"/plugins/CopyEngine/Ultracopier-0.4/
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
	
	insinto /usr/share/Ultracopier/CopyEngine/Ultracopier-0.4/
	doins plugins/CopyEngine/Ultracopier-0.4/informations.xml
	doins plugins/CopyEngine/Ultracopier-0.4/libcopyEngine.so
	fperms 0755 /plugins/CopyEngine/Ultracopier-0.4/libcopyEngine.so

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
	doins plugins/Themes/Oxygen/add.png
	doins plugins/Themes/Oxygen/add.png
	doins plugins/Themes/Oxygen/exit.png
	doins plugins/Themes/Oxygen/info.png
	doins plugins/Themes/Oxygen/systray_Caught_Unix.png
	doins plugins/Themes/Oxygen/systray_Semiuncaught_Unix.png
	doins plugins/Themes/Oxygen/systray_Uncaught_Unix.png

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
			insinto /usr/share/Ultracopier/CopyEngine/Ultracopier-0.4/Languages/${Z}/
			doins -r plugins/CopyEngine/Ultracopier-0.4/Languages/${Z}/
			insinto /usr/share/Ultracopier/Themes/Oxygen/Languages/${Z}/
			doins -r plugins/Themes/Oxygen/Languages/${Z}/
		fi
	done
}
