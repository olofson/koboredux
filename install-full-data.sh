#!/bin/sh
RELEASE=v0.7.2

set -ex

if [ !-d "koboreduxassets" ]; then
	git clone git@github.com:olofson/koboreduxassets.git
	cd koboreduxassets
else
	cd koboreduxassets
	git checkout master
	git pull
fi

git checkout ${RELEASE}

cd ..

if [ ! -L full-data ]; then
	ln -s koboreduxassets/data full-data
fi
