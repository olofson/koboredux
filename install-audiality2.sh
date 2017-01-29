#!/bin/sh
set -ex
git clone https://github.com/olofson/audiality2.git
cd audiality2
git checkout v1.9.2
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_SHARED_LIBS=OFF -DUSE_ALSA=NO -DUSE_JACK=NO .. && make && sudo make install
