# Supported feed formats and online feed services
RSS Guard is a modular application which supports plugins. It offers well-maintained and relatively stable [plugin API](https://github.com/martinrotter/rssguard/blob/master/src/librssguard/services/abstract/serviceentrypoint.h) which can be used to add support for various online feed services, extend a way feeds are processed or add totally new functionality to RSS Guard. At this point RSS Guard offers these plugins which are bundled in all installation packages and some of their features are described in detail in this documentation:
* Standard `RSS/RDF/ATOM/JSON` plugin: This is the core plugin of RSS Guard which allows you to user the app like normal standalone feed reader with great features everyone would expect, including `OPML` files export and import or feed metadata fetching. Also podcasts are supported.
* [Tiny Tiny RSS](https://tt-rss.org) plugin: Adds ability to synchronize messages with TT-RSS instances, either self-hosted or via 3rd-party external service.
* [Inoreader](https://www.inoreader.com) plugin: Adds ability to synchronize messages with Inoreader. All you need to do is create free account on their website and start rocking.
* [Nextcloud News](https://apps.nextcloud.com/apps/news) plugin: Nextcloud News is a Nextcloud app which adds feed reader abilities into your Nextcloud instances.
* [Gmail](https://www.google.com/gmail) plugin: Yes, you are reading it right. RSS Guard can be used as very lightweight and simple e-mail client. This plugins uses [Gmail API](https://developers.google.com/gmail/api) and offers even e-mail sending.

All plugins share almost all core RSS Guard's features, including labels, recycle bins, podcasts fetching or newspaper view. They are implemented in a very transparent way, making it easy to maintain them or add new ones.

Usually, plugins have some exclusive functionality, for example Gmail plugin allows user to send e-mail messages. This extra functionality is always accessible via plugin's context menu and also via main menu.

<img src="images/gmail-context-menu.png" width="80%">

If there is interest in other plugins, you might write one yourself or if many people are interested then I might write it for you, even commercially if we make proper arrangements.

## Features found exclusively in `standard RSS` plugin
Standard plugin in RSS Guard offers some features which are specific to it. Of course it supports all news syndication formats which are nowadays used:
* RSS 0.90, 0.91, 0.92, 1.0 (also known as *RDF*), 2.0.
* ATOM 1.0,
* [JSON](https://www.jsonfeed.org).

Standard plugin offers some extra features like export/import of OPML 2.0 files or fetching feed metadata.

OPML files can be exported/imported in simple dialog.

<img src="images/im-ex-feeds.png" width="80%">
<img src="images/im-ex-feeds-dialog.png">

You just select output file (in case of OPML export), check desired feeds and hit `Export to file`.