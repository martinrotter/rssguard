RSS Guard
=========

RSS Guard is simple (yet powerful) feed reader. It is able to fetch the most known feed formats, including RSS/RDF and ATOM. RSS Guard is developed on top of the [Qt library](http://qt-project.org/) and it supports these operating systems:
* Windows XP and newer,
* GNU/Linux,
* OS/2 (eComStation),
* Mac OS X (possibly),
* xBSD (possibly),
* other platforms supported by Qt.

RSS Guard is written in C++. It is pretty fast even with tons of messages loaded. The core features are:
    * multiplatformity,
    * support for all feed formats,
    * simplicity,
    * sweet look & feel,
    * open-source development model based on GNU GPL license, version 3,
    * tabbed interface,
    * integrated web browser + external browser support,
    * desktop integration via tray icon,
    * Qt library is the only dependency,
    * dynamic keyboard shortcuts,
    * no ads, no hidden costs.

Installation
------------
You need to compile RSS Guard if binary distribution is not available for your platform. Basic steps are really simple:
```
cmake -DCMAKE_BUILD_TYPE=release -DCMAKE_INSTALL_PREFIX="C:\Program Files\rssguard"
make install
```
This compiles and installs RSS Guard using Qt 4 on Windows machines. Qt 5 can be used too. Head to [CMakeLists.txt](https://github.com/martinrotter/rssguard/blob/master/CMakeLists.txt) file for more information on this topic.

Other information
-----------------
    * Binaries will be builded for Windows platform by myself. If you want to build binaries for your platform by yourself, I can provide you some support. **File an issue request** in that case. 
    * If you want to have some feature/ehancement implemented in RSS Guard, then **file an issue request**.
    * If you want to translate RSS Guard, then contact me or **file an issue request** too.
    * Make sure to read the Wiki.


I appreciate any constructive actions.
