RSS Guard
=========
Welcome to RSS Guard website. You can find here basic information. Rest is located in [Wiki](https://bitbucket.org/skunkos/rssguard/wiki/Home).

**[Contacts](#markdown-header-contacts) | [Feeds](#markdown-header-feeds-videos) | [Downloads](#markdown-header-downloads) | [Features](#markdown-header-features) | [Philosophy](#markdown-header-philosophy) | [Donate](#markdown-header-donate)**
- - -
Contacts
--------
* [author's e-mail](mailto:rotter.martinos@gmail.com),
* [forums](https://groups.google.com/d/forum/rssguard),
* [IRC channel at freenode](http://webchat.freenode.net/?channels=#rssguard) (My nick is #skunkos, I am not available on IRC all day long.).
- - -
Donate
------
You can [support RSS Guard with tiny amounts of money via PayPal](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=XMWPLPK893VH4). 
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
Person        | Contribution
:-----------: | :-----------
**Elbert Pol**  |  Huge contrubutions, including translating and testing in OS/2 environment.
**Asen Anastassov**  |  Testing, providing great feedback.


Downloads
---------
### Windows
* [all downloads](https://bitbucket.org/skunkos/rssguard/downloads).
### Linux
* [stable releases](http://software.opensuse.org/download.html?project=home%3Askunkos&package=rssguard),
* [development releases](http://software.opensuse.org/download.html?project=home%3Askunkos&package=rssguard-git) (compiled from the master branch of RSS Guard Git repository),
* Archlinux AUR package ([stable-qt5](https://aur.archlinux.org/packages/rssguard/), [stable-qt4](https://aur.archlinux.org/packages/rssguard-qt4/), [development-qt5](https://aur.archlinux.org/packages/rssguard-git/)].

![RSS Guard is 100% clean.](http://www.softpedia.com/_img/softpedia_100_free.png)
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
* import/export of feeds to/from OPML 2.0,
* message filter with regular expressions,
* feed metadata fetching including icons,
* enhanced feed auto-updating with separate time intervals,
* multiple data backend support,
    * SQLite (in-memory DBs too),
    * MySQL.
* “portable” mode support with clever auto-detection,
* feed categorization,
* drap-n-drop for feed list,
* automatic checking for updates,
* ability to backup/restore database or settings,
* fully-featured recycle bin,
* printing of messages and any web pages,
* can be fully controlled via keyboard,
* feed authentication (Digest-MD5, BASIC, NTLM-2),
* handles tons of messages & feeds,
* sweet look & feel,
* fully adjustable toolbars (changeable buttons and style),
* ability to check for updates on all platforms + self-updating on Windows,
* hideable main menu, toolbars and list headers,
* KFeanza-based default icon theme + ability to create your own icon themes,
* fully skinnable user interface + ability to create your own skins,
* “newspaper” view,
* plenty of skins,
* support for "feed://" URI scheme,
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