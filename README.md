RSS Guard
=========

RSS Guard is simple (yet powerful) feed reader. It is able to fetch the most known feed formats, including RSS/RDF ant ATOM. RSS Guard is developed on top of the [Qt library](http://qt-project.org/). RSS Guard supports these operating systems:
 * Windows XP and newer,
 * GNU/Linux,
 * OS/2 (eComStation).

RSS Guard is written in C++. It is pretty fast even with tons of messages loaded. The core features are:
 * multiplatformity,
 * support for all feed formats,
 * simplicity,
 * sweet look & feel,
 * open-source development model based on GNU GPL license, version 3,
 * tabbed interface,
 * integrated web browser + external browser supported.

Installation
------------
You need to compile and install RSS Guard before you can use it. Basic steps are really simple
```
cmake -DCMAKE_BUILD_TYPE=release -DCMAKE_INSTALL_PREFIX="C:\Program Files\rssguard" -DUSE_QT_5=ON
make install
```
This compiles and installs RSS Guard using Qt 5 on Windows machines. For further information head to [CMakeLists.txt](https://github.com/martinrotter/rssguard/blob/master/CMakeLists.txt) file.

Other information
-----------------
 * If you want to have some feature/ehancement implemented in RSS Guard, then **file an issue request**.
 * If you want to translate RSS Guard, then contact me or **file an issue request** too.
 * Make sure to read the Wiki.


I appreciate any constructive actions.
