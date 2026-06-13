/**
 * NOTES_SYNC_Sources.cpp
 *
 * The Arduino IDE only compiles source files in the sketch's top folder, not
 * in subfolders. This single translation unit #includes the vendored engine
 * .cpp file(s) from ENGINE/ so they are compiled in-tree. Same pattern the
 * engine uses with FileGame_Sources.cpp.
 *
 * Header-only vendored files (FileOperator.h, ESP32FileOperator.h,
 * GraphicsRenderer.h) need no entry here.
 */

#include "ENGINE/ESP32GraphicsRenderer.cpp"
