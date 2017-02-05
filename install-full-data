#!/bin/sh
. ./PACKAGEDEFS

set -ex

# Clone or update repository as needed
if [ ! -d "koboreduxassets" ]; then
	git clone git@github.com:olofson/koboreduxassets.git
	cd koboreduxassets
else
	cd koboreduxassets
	git checkout master
	git pull
fi

# Checkout the appropriate release
git checkout v${KRRELEASE}

# Render the TIPE sprites
cd src
eel render-sprites
cd ..

# Make sure we have a link to the actual data directories
cd ..
if [ ! -L full-data ]; then
	ln -s koboreduxassets/data full-data
fi
