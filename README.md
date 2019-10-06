Tilemap Helper: Gimp Plug-in for Optimizing Tile Maps and Tile Sets
===========

NOTE: The current version is rough around the edges. It needs improvement and refactoring of the source. That may come in time. For now, consider it a mostly working beta...

A GIMP plugin to help with creating tile maps and tile sets for games.

You can create a level map in Gimp (or other image editors) and use this plugin to help with tile de-duplication and tile set optimizing.

This plugin is not meant to replace tile set and map editors. Instead it aims to provide quick, in-workflow feedback without having to leave GIMP to analyze tiles in another application.

Download compiled executables here: 
 * [Linux GIMP 2.8+](/bin/linux)
 * [Windows GIMP 2.10.12+](/bin/windows)


![Plugin showing tile map helper with an image open](/info/Screenshot.png)


Features
 * Optional Tile deduplication
 * Overlay of Tile ID # on source image
 * Click-to-highlight matching tiles on source image
 * Estimates of memory usage for storing Tile Set and Map
 * Use either Source Layer or Entire Image
 * Variable Tile size
 * Tile X/Y Flipping detection
 * Export Tile Set as image -> new GIMP image
 * Export Tile Map as text -> Clipboard (C array, RGBDS ASM)
 * Works with indexed and 24 bit RGB images (including alpha masks)


OS binaries available for:
 * Linux (GIMP 2.8+)
 * Windows (GIMP 2.10.12+)


## Usage:

* The plugin is located under: Main Menu -> Filters -> Map -> Tilemap Helper

## Quick instructions:

Native compile/install on Linux using below.

```
If GIMP & build tools not yet installed:
(example for debian/ubuntu/mint)
 * sudo apt install gimp
 * sudo apt install build-essential
 * sudo apt install libgimp2.0-dev
 
Then: 
* cd gimp-rom-bin
* make

Then copy the resulting "plugin-gimp-tilemap-helper" to your GIMP plugin folder, depends on version

Plug-in folder locations:
 Linux: ~/.gimp-2.8/plug-ins  , or ~/.config/GIMP/2.10/plug-ins
 Windows: C:\Program Files\GIMP 2\lib\gimp\2.0\plug-ins

Guide for [Cross-compiling to Windows on Linux](https://github.com/bbbbbr/gimp-rom-bin/blob/master/doc/GIMP%20jhbuild%20for%20Windows%20on%20Linux.md)


```

## Requirements:

## Known limitations & Issues:
* Max number of tiles: 8096
* The Source Image or Layer must be an exact multiple of tile size in both dimensions
* Greyscale images are not yet supported. Convert to RGB or indexed first
* Map export prefix labels are saved to images as GimpParasites, so only persist across sessions when images are saved in GIMP's native XCF format

## GIMP usage hints:
