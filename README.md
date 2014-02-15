RSS Guard
=========

Features
--------
RSS Guard is simple (yet powerful) feed reader. It is able to fetch the most known feed formats, including RSS/RDF and ATOM. RSS Guard is developed on top of the [Qt library](http://qt-project.org/) and it supports these operating systems:

* Windows XP and newer,
* GNU/Linux,
* OS/2 (eComStation),
* Mac OS X,
* xBSD (possibly),
* other platforms supported by Qt.

RSS Guard is written in C++. It is pretty fast even with tons of messages loaded. The core features are:

* multiplatformity,
* support for all feed formats,
* simplicity,
* feed metadata fetching,
* multiple data backend support (SQLite + MySQL),
* “portable” mode support,
* feed categorization,
* feed authentication (Digest-MD5, BASIC, NTLM-2),
* handles tons of messages,
* sweet look & feel,
* KFeanza-based default icon theme + ability to create your own icon themes,
* fully skinnable user interface + ability to create your own skins,
* “newspaper” view,
* ability to hide list of feeds/categories,
* open-source development model based on GNU GPL license, version 3,
* tabbed interface,
* integrated web browser + external browser support,
* desktop integration via tray icon,
* Qt library is the only dependency,
* no ads, no hidden costs.

Installation
------------
You need to compile RSS Guard if binary distribution is not available for your platform. Basic steps are really simple:
```
cmake -DCMAKE_BUILD_TYPE=release -DCMAKE_INSTALL_PREFIX="C:\Program Files\rssguard"
make install
```
This compiles and installs RSS Guard using Qt 4 on Windows machines. Qt 5 can be used too.