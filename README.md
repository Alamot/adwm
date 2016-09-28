Alamot's DWM - Dynamic Window Manager
======================================
This is my version of dwm. DWM is an extremely fast, small, and dynamic window manager for X.

## Extra features included in Alamot's DWM:
* Alpha patch (aka transparency)
* Better key assignment (e.g. tags on numpad keys)
* Change keyboard layout using the mouse buttons
* Pertag patch
* Runorraise patch
* Query mouse pointer focus
* Status2d patch
* Systray patch
* Three different clickable ClkStatusText areas (Left/Middle/Right)
* Viewontag patch
* Xft support
* XPM icons/pictures support
* Zoomswap patch

![Busy](/29Aug2016_00:31:02_1280x1024.jpg "Busy")

## Status bar indicators:
* Keyboard layout (with country flags)
* Load average
* CPU temperature
* Available memory
* Wifi connection status, signal and txrate using the nl80211 interface (with graphic representation)

![Wifi indicator](/wifi.png "Wifi indicator")
====================================================

Requirements
------------
* glibc
* libx11
* libxft
* libfontconfig
* libxpm         (for xpm icons/pictures)
* libnl-genl-3.0 (for nl80211 wifi indicator)

Installation
------------
Edit config.mk to match your local setup (dwm is installed into
the /usr/local namespace by default).

Afterwards enter the following command to build and install dwm (if
necessary as root):

    make clean install

Running dwm
-----------
Copy dwm_start.sh in your home directory and add the following line to
your .xinitrc to start dwm using startx:

    exec dwm_start.sh

In order to connect dwm to a specific display, make sure that
the DISPLAY environment variable is set correctly, e.g.:

    DISPLAY=foo.bar:1 exec dwm

(This will start dwm on display :1 of the host foo.bar.)

Configuration
-------------
The configuration of dwm is done by creating a custom config.h
and (re)compiling the source code.
