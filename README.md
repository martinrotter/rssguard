RSS Guard
=========

[![Build status](https://img.shields.io/travis/martinrotter/rssguard.svg?maxAge=360)](https://travis-ci.org/martinrotter/rssguard)
[![Total downloads](https://img.shields.io/github/downloads/martinrotter/rssguard/total.svg?maxAge=360)](#)
[![Version](https://img.shields.io/github/release/martinrotter/rssguard.svg?maxAge=360)](#)
[![GitHub issues](https://img.shields.io/github/issues/martinrotter/rssguard.svg?maxAge=360)](#)
[![AUR](https://img.shields.io/aur/votes/rssguard.svg?maxAge=3600)](https://aur.archlinux.org/packages/rssguard/)
[![License](https://img.shields.io/github/license/martinrotter/rssguard.svg?maxAge=360000)](#)
[![Maintenance](https://img.shields.io/maintenance/yes/2016.svg?maxAge=2592000)](#)

Welcome to RSS Guard website. You can find here basic information.

RSS Guard is simple, light and easy-to-use RSS/ATOM feed aggregator developed using Qt framework which supports online feed synchronization.

Core features are:

* **support for online feed synchronization via plugins**,
    * Tiny Tiny RSS (from RSS Guard 3.0.0),
    * ownCloud News (from RSS Guard 3.1.0).
* multiplatformity,
* server/client cron-like mode,
* multiple data backend support.
    * SQLite (in-memory DBs too),
    * MySQL.

See below for more information about features and other RSS Guard aspects. Also visit Wiki for more detailed information or tutorials.

Contacts
--------
* [author's e-mail](mailto:rotter.martinos@gmail.com),
* [IRC channel at freenode](http://webchat.freenode.net/?channels=#rssguard) (My nick is #skunkos, I am not available on IRC all day long.).

Donate
------
You can [support RSS Guard with tiny amounts of money via PayPal](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=XMWPLPK893VH4).

[![Support RSS Guard now.](http://manlybeachrunningclub.com/wp-content/uploads/2015/01/paypal-donate-button115.png)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=XMWPLPK893VH4)

People who donated:

* Zdenek S. (Sweden)
* Eloi Garibaldi B.
* Jacob S.

I say "thank you" for all your support, donators.

Feeds & Videos
-----
* [video channel at YouTube](http://www.youtube.com/playlist?list=PL-75mFFA3wujyMyea6W1qJEV_ulh6433j),
* [screenshots](https://drive.google.com/folderview?id=0B8XNkQ-jUoBYdVRSNm1kQ3BUMzQ&usp=sharing).

Thanks to
-----
* **Elbert Pol** - huge contrubutions, including translating and testing in OS/2 environment.
* **Asen Anastassov** - testing, providing great feedback.
* **eson** - rock-solid translation into Sweden language.

Downloads
---------
* [official downloads](https://github.com/martinrotter/rssguard/releases),
* alternative downloads:
  * [Archlinux AUR package](https://aur.archlinux.org/packages/rssguard/),
  * [OBS/development releases](https://build.opensuse.org/package/show/home:skunkos:rssguard/rssguard-git) (click "Download package" in top/right corner of the website).

[![Alternative RSS Guard downloads.](http://www.instalki.pl/img/buttons/en/download_dark.png)](http://www.instalki.pl/programy/download/Windows/czytniki_RSS/RSS_Guard.html)

Note that packages from OBS are **NOT** meant to be used in production. Their main purpose is to control if RSS Guard can be build without errors on those distros. Rely on "official" packages for your Linux distro made by your Linux distro maintainers.

![RSS Guard is 100% clean.](http://www.softpedia.com/_img/softpedia_100_free.png)

Features
--------
RSS Guard is simple (yet powerful) feed reader. It is able to fetch the most known feed formats, including RSS/RDF and ATOM. RSS Guard is developed on top of the [Qt library](http://qt-project.org/) and it supports these operating systems:

* Windows XP and newer,
* GNU/Linux,
* Mac OS X,
* xBSD (possibly),
* Android (possibly),
* other platforms supported by Qt.

RSS Guard is written in C++. It is pretty fast even with tons of messages loaded. The core features are:

* **support for online feed synchronization via plugins**,
    * Tiny Tiny RSS (from RSS Guard 3.0.0),
    * ownCloud News (from RSS Guard 3.1.0).
* multiplatformity,
* support for all feed formats,
* simple internal Chromium-based web viewer,
* simplicity,
* server/client cron-like mode,
* import/export of feeds to/from OPML 2.0,
* downloader with own tab and support for up to 6 parallel downloads,
* message filter with regular expressions,
* very fast parallelized feed updates,
* feed metadata fetching including icons,
* no crazy dependencies,
* ability to cleanup internal message database with various options,
* enhanced feed auto-updating with separate time intervals,
* multiple data backend support,
    * SQLite (in-memory DBs too),
    * MySQL.
* is able to specify target database by its name (MySQL backend),
* “portable” mode support with clever auto-detection,
* feed categorization,
* drap-n-drop for feed list,
* automatic checking for updates,
* full support of podcasts (both RSS & ATOM),
* ability to backup/restore database or settings,
* fully-featured recycle bin,
* can be fully controlled via keyboard,
* feed authentication (Digest-MD5, BASIC, NTLM-2),
* handles tons of messages & feeds,
* sweet look & feel,
* fully adjustable toolbars (changeable buttons and style),
* ability to check for updates on all platforms + self-updating on Windows,
* hideable main menu, toolbars and list headers,
* Feanza-based default icon themes,
* fully skinnable user interface + ability to create your own skins,
* newspaper view,
* support for "feed://" URI scheme,
* ability to hide list of feeds/categories,
* open-source development model based on GNU GPL license, version 3,
* tabbed interface,
* desktop integration via tray icon,
* localizations to some languages,
* Qt library is the only dependency,
* open-source development model and friendly author waiting for your feedback,
* no ads, no hidden costs.

Philosophy
----------
RSS Guard tends to be independent software. It's free, it's open-source. RSS Guard accepts donations but only to SUPPORT its development.
