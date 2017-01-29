#!/bin/sh
set -ex
wget https://github.com/olofson/koboredux/releases/download/demo-0.7.1/KoboRedux-Demo-0.7.1-Data.tar.bz2
tar -xjvf KoboRedux-Demo-0.7.1-Data.tar.bz2
mv KoboRedux-Demo-0.7.1-Data demo-data
