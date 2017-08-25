Kobo Redux
==========

This file explains how to build and install Kobo Redux from source. It does NOT cover building of packages, but the executables built from this source tree ARE compatible with the proprietary data files; both demo and full version.

*NOTE*
------

*The proprietary sound and graphics themes are not included in the source tree! However, placeholder GPL themes "chip" (sound) and "mono" (graphics) are included, making the game mostly playable with no external data.*

Installing
----------

* Install the dependencies. Note that Audiality 1.9.x is a development branch, which means any given release of Kobo Redux will most likely only work with one specific Audiality version.
  * SDL 2.0
    * http://libsdl.org
  * Audiality 2 (currently 1.9.2)
    * https://github.com/olofson/audiality2
  * *Optional:* MXE (needed for cross-compiling Windows binaries)
    * http://mxe.cc/

* Download the source code.
  * GitHub/SSH
    * git clone git@github.com:olofson/koboredux.git
  * *Alternatively:* GitHub/HTTPS
    * git clone https://github.com/olofson/koboredux.git
  
* Configure the source tree.
  * Option 1 (Un*x with bash or compatible shell)
    * ./configure [*target*]
      * Currently available targets:
        * release
        * demo
        * maintainer
        * debug
        * mingw-release
        * mingw-demo
        * mingw-debug
        * all *(all of the above)*
        * (Default if not specified: release + demo)
  * Option 2 (Using CMake directly; see 'configure' script to see how to set up the options)
    * mkdir build
    * cd build
    * cmake ..

* Build and install.
  * ./make-all
    * (The resulting executables are found in "build/<target>/src/")
  * *Alternatively:* Enter the desired build target directory.
    * make
    * *Optional:* sudo make install
