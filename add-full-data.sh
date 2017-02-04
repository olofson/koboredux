#!/bin/sh
#
# This script downloads the full version DEB, RPM and Bzip2 packages from
# GitHub, unpacks them, adds the full version data files, and rebuilds complete
# full version packages.
#
set -ex

. ./PACKAGEDEFS

SRCDIR="$(pwd)"
OUTDIR="${SRCDIR}/full-packages"
PACKAGENAME="koboredux-${KRRELEASE}-Linux"
DEBARCH="${PACKAGENAME}.deb"
RPMARCH="${PACKAGENAME}.rpm"
BZIP2ARCH="${PACKAGENAME}.tar.bz2"
BASEURL="${KRREPO}/releases/download/v${KRRELEASE}"

DATADIR="usr/share/koboredux"

#
# Common
#

prepare_rebuild() {
	# Make sure we have the appropriate full version data files
	./install-full-data.sh

	# Clean up or create target directory, as needed
	if [ -d "${OUTDIR}" ]; then
		rm -rf ${OUTDIR}/*
	else
		mkdir ${OUTDIR}
	fi

	# Grab the respective packages from the public GitHub release
	cd ${OUTDIR}
	wget "${BASEURL}/${BZIP2ARCH}"
	wget "${BASEURL}/${DEBARCH}"
	wget "${BASEURL}/${RPMARCH}"
	cd ${SRCDIR}
}

add_data_files() {
	local target=$1
	cp -r ../full-data/* "${target}"
	awk '{ sub("\r$", ""); print }' ../full-data/EULA.txt > "${target}/EULA.txt"
}

fix_permissions() {
	# Fix bogus permissions
	local target=$1
	find ${target} -type d -print0 | xargs -0 chmod 755
	find ${target}/usr/share -type f -print0 | xargs -0 chmod 644
	find ${target}/usr/bin -type f -print0 | xargs -0 chmod 755
}


#
# Bzip2
#

rebuild_bzip2() {
	# Unpack
	cd ${OUTDIR}
	tar -xjvf ${BZIP2ARCH}
	rm -rf ${BZIP2ARCH}

	add_data_files "${PACKAGENAME}/${DATADIR}"

	fix_permissions "${PACKAGENAME}"

	# Repackage and clean up
	BZIP2=--best tar -cvjSf ${BZIP2ARCH} ${PACKAGENAME}
	rm -rf ${PACKAGENAME}
	cd ${SRCDIR}
}


#
# DEB
#

rebuild_deb() {
	# Unpack
	cd ${OUTDIR}
	mkdir pkg
	dpkg-deb -R ${DEBARCH} pkg
	rm ${DEBARCH}

	add_data_files "pkg/${DATADIR}"

	cd pkg

	# Generate new md5sums
	find . -type f ! -regex '.*.hg.*' ! -regex '.*?debian-binary.*' ! -regex '.*?DEBIAN.*' -printf '%P ' | xargs md5sum > DEBIAN/md5sums
	chmod 644 DEBIAN/md5sums

	fix_permissions .

	# Repackage and clean up
	cd -
	dpkg-deb -b pkg ${DEBARCH}
	rm -rf pkg
	cd ${SRCDIR}
}


#
# RPM
#

create_rpm_spec() {
	local package=$1
	local spec=$2
	rpm -qp --qf "Name: %{NAME}
Version: %{VERSION}
Release: %{RELEASE}
BuildArch: %{ARCH}
Group: %{GROUP}
License: %{LICENSE}
Source RPM: %{SOURCERPM}
Vendor: Olofson Arcade
Packager: David Olofson <david@olofson.net>
URL: %{URL}
Summary: %{SUMMARY}

%%description
%{DESCRIPTION}

%%files
/usr/bin/*
/usr/share/doc/*
/usr/share/kobo*/*
/usr/share/applications/*
/usr/share/pixmaps/*
" ${package} > ${spec}
}

rebuild_rpm() {
	# Unpack
	cd ${OUTDIR}
	mkdir pkg
	cd pkg
	rpm2cpio ../${RPMARCH} | cpio -idmv
	cd ..

	add_data_files "pkg/${DATADIR}"

	fix_permissions pkg

	create_rpm_spec ${RPMARCH} koboredux.spec
	echo "--- Generated spec ---"
	cat koboredux.spec
	echo "----------------------"

	# Repackage and clean up
	archdir=$(rpm -qp --qf "%{ARCH}" ${RPMARCH})
	rm ${RPMARCH}
	rpmbuild --buildroot "$(pwd)/pkg" --define "_rpmdir $(pwd)" -bb koboredux.spec
	mv ${archdir}/*.rpm ${RPMARCH}
	rm -rf ${archdir}
	rm koboredux.spec
	cd ${SRCDIR}
}


#
# Main
#

# Some packages need to be built as (fake)root, so we wrap ourselves...
if [ $# -eq 0 ]; then
	prepare_rebuild
	rebuild_bzip2
	rebuild_rpm
	fakeroot ./$0 deb
else
	case "$1" in
		deb)
			rebuild_deb
			;;
	esac
fi
