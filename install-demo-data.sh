#!/bin/sh
DEMOARC="KoboRedux-Demo-0.7.2-Data.tar.bz2"

if [ -d "demo-data" ]; then
	echo "Demo data files (demo-data) already in place!"
	exit
fi

set -ex

mkdir demo-data
cd demo-data
wget "http://koboredux.com/download/${DEMOARC}"
tar -xjvf ${DEMOARC} --strip-components=1
rm ${DEMOARC}
