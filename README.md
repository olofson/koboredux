Kobo Redux
==========

Kobo Deluxe Revived
-------------------

Kobo Redux is a frantic 80's style 2D shooter with the look and feel a 90's arcade cabinet, the smoothness of current technology, and a matching soundtrack. The gameplay is fast, intense and unforgiving, while trying to tone down the frustrating quirkiness of the actual games of the 80's. A true challenge in the spirit of the arcade era!

First, inspired by Bosconian, there was XKobo, a frantic multi-directional scrolling arcade shooter, where the primary objective was to destroy maze-like hostile space bases. Kobo Deluxe brought this addictive game to basically everything with a CPU - but it was never quite finished. Now, with smoother controls, improved gameplay, new pixel art, and a proper soundtrack, Kobo Redux aims to be the XKobo update that Kobo Deluxe never quite became.

While Kobo Redux is a proprietary title, it is implemented using portable Free/Open Source technologies, and the full game source code will be made available as the game is released.

*NOTE*
------
*The proprietary sound and graphics themes are not included in this repository! However, placeholder GPL themes "chip" (sound) and "mono" (graphics) are included, making the game fully playable with no external data.*

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
