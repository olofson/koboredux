#!/bin/sh
REDISTARCH="koboredux-libs-redist-win32.tar.bz2"
A2RELEASE=v1.9.2

if [ -d "win32-redist" ]; then
	echo "Win32 redistributables (win32-redist) already in place!"
	exit
fi

set -ex

mkdir win32-redist
cd win32-redist

# Grab Win32 DLL package from koboredux.com
wget "http://koboredux.com/download/${REDISTARCH}"
tar -xjvf ${REDISTARCH} --strip-components=1
rm ${REDISTARCH}

# Grab Audiality2 DLL and COPYING file from GitHub
wget "https://github.com/olofson/audiality2/releases/download/${A2RELEASE}/libaudiality2.dll"
wget https://raw.githubusercontent.com/olofson/audiality2/master/COPYING
awk 'sub("$", "\r")' COPYING > LICENSE-Audiality2.txt
rm COPYING
