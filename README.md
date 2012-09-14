snes9x-3d
=========

snes9x + stereoscopic graphics

Based on snes9x 1.53 source
Currently, only works on gtk gui (without xv, opengl) and odd/even rows interleaved displays.
Also you need to uncheck "scale image to fit window" (in Preferences-> Display -> Image Adjustments).
I tested it without using threads and with HQ4x filter. 

To build the project, simply
cd gtk; make

Alternatively, you can 
cd gtk; configure --without-xv --without-opengl; make

