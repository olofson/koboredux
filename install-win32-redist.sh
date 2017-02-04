#!/bin/sh

if [ -d "win32-redist" ]; then
	echo "Win32 redistributables (win32-redist) already in place!"
	exit
fi

set -ex

mkdir win32-redist

# Grab Win32 DLL package from koboredux.com
wget http://koboredux.com/download/koboredux-libs-redist-win32.tar.bz2
cd win32-redist
tar -xjvf ../koboredux-libs-redist-win32.tar.bz2 --strip-components=1

# (NOTE: We're still in win32-redist...)

# Grab Audiality2 DLL and COPYING file from GitHub
wget https://github.com/olofson/audiality2/releases/download/v1.9.2/libaudiality2.dll
wget https://raw.githubusercontent.com/olofson/audiality2/master/COPYING
awk 'sub("$", "\r")' COPYING > LICENSE-Audiality2.txt
rm COPYING
