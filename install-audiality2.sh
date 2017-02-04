#!/bin/sh
. ./PACKAGEDEFS

if pkg-config audiality2 --exists; then
	echo "Audiality 2 already installed on the system!"
	exit
fi

set -ex
if [ ! -d "audiality2" ]; then
	git clone https://github.com/olofson/audiality2.git
	cd audiality2
else
	cd audiality2
	git pull
fi
git checkout v${A2RELEASE}
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_SHARED_LIBS=OFF -DUSE_ALSA=NO -DUSE_JACK=NO .. && make && sudo make install
