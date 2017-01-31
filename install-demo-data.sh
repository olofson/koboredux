#!/bin/sh
set -ex
wget https://github.com/olofson/koboredux/releases/download/demo-0.7.2/KoboRedux-Demo-0.7.2-Data.tar.bz2
tar -xjvf KoboRedux-Demo-0.7.2-Data.tar.bz2
mv KoboRedux-Demo-0.7.2-Data demo-data
