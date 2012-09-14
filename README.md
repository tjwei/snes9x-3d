snes9x-3d
=========

snes9x + stereoscopic graphics

Based on snes9x 1.53 source
Currently, only works on gtk gui (without xv, opengl) and odd/even rows interleaved displays.

To build the project, simply
cd gtk; make

Alternatively, you can 
cd gtk; configure --without-xv --without-opengl; make

