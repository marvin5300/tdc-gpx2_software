# Software for RasPi compatible shield for TDC-GPX2 chip
The TDC-GPX2 is a time-to-digital converter from ScioSense with up to 10ps timing accuracy. This software uses the linux kernel for GPIO and SPI-Bus to readout the GPX2 and calculate time intervals between signals.

Required:
- linux operating system (preferrably raspberry pi OS)
- up-to-date c++ compiler (C++17 compatible), tested with clang 11 and newer

How to build:
- This software packages is build with cmake
- builds the gpx2 library and optionally the readout program
