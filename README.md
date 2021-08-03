# Software for RasPi compatible shield for TDC-GPX2 chip
The TDC-GPX2 is a time-to-digital converter from ScioSense with up to 10ps timing accuracy. This software uses the linux kernel for GPIO and SPI-Bus to readout the GPX2 and calculate time intervals between signals. There is also the possibility of reading out the TDC using a FPGA which will then give much faster readout.

# TDC-GPX2_Software
Files for building readout software for the TDC-GPX2 from ScioSense.
| Folder | Description |
| ------ | ----------- |
| source/gpx2-raspi-readout-program| Contains a readout loop, buffer and calculations to measure time intervals between two signals with good accuracy.
| source/spi | Folder containing gpx2 source files for making the tdc board library containing all options and an high level access for readout.
| source/gpx2-fpga | tba |
<br>
<strong>Readout via SPI:</strong>

Required:
- linux operating system (preferrably raspberry pi OS)
- up-to-date c++ compiler (C++17 compatible), tested with clang 11 and newer

How to build:
- This software packages is build with cmake
- builds the gpx2 library and optionally the readout program

<br>
<strong>Readout via LVDS using an FPGA:</strong><br>
coming soon
