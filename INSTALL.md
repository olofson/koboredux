Kobo Redux
==========

This file explains how to build and install Kobo Redux from source. It does not apply to binary packages.

*NOTE*
------

*The proprietary sound and graphics themes are not included in the source tree! However, placeholder GPL themes "chip" (sound) and "mono" (graphics) are included, making the game mostly playable with no external data.*

Installing
----------

* Install the dependencies. Note that Audiality 1.9.x is a development branch, which means any given release of Kobo Redux will most likely only work with one specific Audiality version.
  * SDL 2.0
    * http://libsdl.org
  * Audiality 2 (currently 1.9.1)
    * https://github.com/olofson/audiality2
  * *Optional:* MXE (needed for cross-compiling Windows binaries)
    * http://mxe.cc/

* Download the source code.
  * GitHub
    * git clone git@github.com:olofson/koboredux.git
  
* Configure the source tree.
  * Option 1 (Un*x with bash or similar shell)
    * ./cfg-all
  * Option 2 (Using CMake directly)
    * mkdir build
    * cd build
    * cmake ..

* Build and install.
  * Enter the desired build directory. (cfg-all creates a few different ones under "build".)
  * make
  * *Optional:* sudo make install
