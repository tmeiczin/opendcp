OpenDCP is an open source program to create digital cinema packages (DCP) for use in digital cinema.

## PLEASE do not post general questions in the issues area. Questions can be asked at [opendcp](http://groups.google.com/group/opendcp) or [dcinemaforum](http://dcinemaforum.com). ##

**Latest News**

Helmut Lottenburger has created an [online course](https://www.udemy.com/digital-cinema/?couponCode=dciforum36) covering aspects of Digital Cinema mastering. It's not a step by step tutorial on using tools like OpenDCP, but rather an overview of the digital cinema workflow. You save $10 by using this [link](https://www.udemy.com/digital-cinema/?couponCode=dciforum36) and Helmut will donate a percentage of the course fee to the OpenDCP project.

**Version 0.30.0**
  * Added MXF Encryption (with code contributions by Lars G)
  * Added additional XYZ conversion algorithms (Addresses issues with elevated blacks from certain applications)
  * Fixed [Issue 219](https://code.google.com/p/opendcp/issues/detail?id=219): Error in making a video MXF
  * Fixed [Issue 221](https://code.google.com/p/opendcp/issues/detail?id=221): OpenDCP crashes when rating is selected
  * Fixed [Issue 224](https://code.google.com/p/opendcp/issues/detail?id=224): Add Slovenian language
  * Fixed [Issue 232](https://code.google.com/p/opendcp/issues/detail?id=232): Command-line opendcp\_mxf with slides crashes
  * Fixed [Issue 233](https://code.google.com/p/opendcp/issues/detail?id=233): DCI image check was not complete
  * Fixed [Issue 236](https://code.google.com/p/opendcp/issues/detail?id=236): HASH field order
  * Fixed [Issue 238](https://code.google.com/p/opendcp/issues/detail?id=238): Typo in cmakelist.txt
  * Fixed [Issue 240](https://code.google.com/p/opendcp/issues/detail?id=240): I have non-ascii charactere error since 0.29 [Issue 241](https://code.google.com/p/opendcp/issues/detail?id=241): No warning for non-DCI image resolutions
  * Fixed [Issue 242](https://code.google.com/p/opendcp/issues/detail?id=242): MXF audio file not copied/moved

**Version 0.29.0**
  * Added support to read JPEG2000 images and re-encode to DCI compliant versions
  * Fixed [Issue 205](https://code.google.com/p/opendcp/issues/detail?id=205):  Subtitle hashes not being added to DCP in GUI
  * Fixed [Issue 156](https://code.google.com/p/opendcp/issues/detail?id=156):	CLI crash when using folders for in and output
  * Fixed [Issue 168](https://code.google.com/p/opendcp/issues/detail?id=168):	Non ASCII characters in filename, wrong error message.
  * Fixed [Issue 184](https://code.google.com/p/opendcp/issues/detail?id=184):	Duration miscalculation
  * Fixed [Issue 171](https://code.google.com/p/opendcp/issues/detail?id=171):	Image order incorrect when creating mxf when using opendcp\_mxf command line tool (and also 173, 206)
  * Fixed  [Issue 213](https://code.google.com/p/opendcp/issues/detail?id=213):	opendcp\_j2k command failing with long names

**Version 0.28.1**
  * Fixed memory leak bug during jpeg2000 encoding

**Version 0.28.0**
  * Updated to latest asdcplib
  * Fixed a bunch of field ordering issues in XML
  * Fixed slideshow in opendcp\_mxf
  * Fixed Windows 32-bit version
  * Fixed block align in wav MXF
  * Other various fixes

**Features**
  * JPEG2000 encoding from 8/12/16-bit TIFF/DPX RGB/YUV/YCbCr images
  * Supports all major frame rates (24,25,30,48,50,60)
  * Cinema 2K and 4K
  * MPEG2 MXF
  * XYZ color space conversion
  * MXF file creation
  * SMPTE and MXF Interop
  * Full 3D support
  * DCP XML file creation
  * SMPTE subtitles
  * Linux/OSX/Windows
  * Multithreaded for encoding performance
  * XML Digital signatures
  * GUI

**Future Features**

This is released as Beta code and in active development, so expect some issues. Any bugs or enhancements should be added to the issues section.

[FAQ](http://code.google.com/p/opendcp/wiki/FAQ) - Updated

[Documentation](http://code.google.com/p/opendcp/wiki/Documentation)

[Download](http://code.google.com/p/opendcp/downloads/list)

[![](https://www.paypal.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=FSTXMPDXE3VCJ)