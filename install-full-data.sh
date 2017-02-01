#!/bin/sh
set -ex
git clone git@github.com:olofson/koboreduxassets.git
cd koboreduxassets
git checkout v0.7.2
cd ..
ln -s koboreduxassets/data full-data
