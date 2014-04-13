RSS Guard
=========
Welcome to RSS Guard website. You can find here basic information. Rest is located in [Wiki](https://bitbucket.org/skunkos/rssguard/wiki/Home).

**[Contacts](#markdown-header-contacts) | [Feeds](#markdown-header-feeds) | [Downloads](#markdown-header-downloads) | [Features](#markdown-header-features) | [Philosophy](#markdown-header-philosophy)**
- - -
Contacts
--------
* [author's e-mail](mailto:rotter.martinos@gmail.com),
* [forums](https://groups.google.com/d/forum/rssguard),
* [IRC channel at freenode](http://webchat.freenode.net/?channels=#rssguard) (nick of author is #skunkos).
- - -
Feeds & Videos
-----
* [video channel at YouTube](http://www.youtube.com/playlist?list=PL-75mFFA3wujyMyea6W1qJEV_ulh6433j),
* [RSS channel at Google Groups](https://groups.google.com/forum/feed/rssguard/msgs/rss_v2_0.xml?num=50),
* [RSS channel at BitBucket](https://bitbucket.org/skunkos/rssguard/rss),
* [screenshots](https://drive.google.com/folderview?id=0B8XNkQ-jUoBYdVRSNm1kQ3BUMzQ&usp=sharing).
- - -
Thanks to
-----
Person       | Contribution
-----------: | :-----------
**Elbert Pol**   |  Huge contrubutions, including translating, testing. He is also maintainer of OS/2 RSS Guard package.


Downloads
---------
You can download source tarballs or binaries for some platforms in [Downloads](downloads) section of this website. Precompiled binaries for particular Linux distributions are available too:

* [stable releases](http://software.opensuse.org/download.html?project=home%3Askunkos&package=rssguard),
* [development releases](http://software.opensuse.org/download.html?project=home%3Askunkos&package=rssguard-git) (compiled from the master branch of RSS Guard Git repository).
- - -
Features
--------
RSS Guard is simple (yet powerful) feed reader. It is able to fetch the most known feed formats, including RSS/RDF and ATOM. RSS Guard is developed on top of the [Qt library](http://qt-project.org/) and it supports these operating systems:

* Windows XP and newer,
* GNU/Linux,
* OS/2 (eComStation),
* Mac OS X,
* xBSD (possibly),
* Android (possibly),
* other platforms supported by Qt.

RSS Guard is written in C++. It is pretty fast even with tons of messages loaded. The core features are:

* multiplatformity,
* support for all feed formats,
* simplicity,
* feed metadata fetching including icons,
* enhanced feed auto-updating with separate time intervals,
* multiple data backend support (SQLite + MySQL),
* “portable” mode support,
* feed categorization,
* feed authentication (Digest-MD5, BASIC, NTLM-2),
* handles tons of messages & feeds,
* sweet look & feel,
* fully adjustable toolbars (changeable buttons and style),
* ability to check for updates on all platforms + self-updating on Windows and OS/2,
* hideable main menu,
* KFeanza-based default icon theme + ability to create your own icon themes,
* fully skinnable user interface + ability to create your own skins,
* “newspaper” view,
* ability to hide list of feeds/categories,
* open-source development model based on GNU GPL license, version 3,
* tabbed interface,
* integrated web browser with adjustable behavior + external browser support,
* internal web browser mouse gestures support,
* desktop integration via tray icon,
* localizations to some languages,
* Qt library is the only dependency,
* open-source development model and friendly author waiting for your feedback,
* no ads, no hidden costs.
- - -
Philosophy
----------
RSS Guard tends to be independent software. It's free, it's open-source. RSS Guard will never depend on other services - this includes online news aggregators like Feedly, The Old Reader and others.

That's why RSS Guard will never integrate those services unless someone else codes support for them on his own. Remember, RSS Guard supports online synchronization via MySQL/MariaDB or you can use Dropbox to synchronize SQLite data storage.