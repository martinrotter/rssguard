Supported Feed Readers
======================
RSS Guard is a multi-account application and supports many web-based feed readers via built-in plugins. Its `standard` plugin provides support for these feed formats:
* RSS
* ATOM
* RDF
* [XMPP](features/xmpp)[^5]
* [iCalendar](https://en.wikipedia.org/wiki/ICalendar)
* [JSON](https://www.jsonfeed.org)
* [Sitemap](https://en.wikipedia.org/wiki/Sitemaps) (including sitemap index discovery and compressed sitemaps)

RSS Guard also has built-in support for the [Gemini](https://geminiprotocol.net) protocol, so it can fetch regular feeds over Gemini without any problems.

When it comes to online web-based feed readers, these are supported:

| Service | Two-way Synchronization | Intelligent Synchronization Algorithm (ISA) [^1] | Synchronized Labels [^2] | OAuth [^3] |
| :---              | :---:  | :---: | :---: | :---: |
| Feedly            | Yes | Yes | Yes | Yes (only for official binaries) |
| Gmail             | Yes | Yes | Yes | Yes |
| Google Reader API [^4] | Yes | Yes | Yes | Yes (only for Inoreader) |
| Nextcloud News    | Yes | No | No | No |
| Tiny Tiny RSS     | Yes | Yes | Yes | No |

[^1]: Some plugins support the next-generation intelligent synchronization algorithm (ISA), which has several benefits. It usually offers better synchronization speed and transfers much less data over your network connection.

    With ISA, RSS Guard only downloads articles that are new or were updated by the remote server. The old algorithm usually fetches all available articles, even if they are not needed, which leads to unnecessary load on both your network connection and RSS Guard.

[^2]: Note that [labels](#features/labels) are supported for all plugins, but for some plugins they are local-only and are not synchronized with the service, usually because the service itself does not support the feature.

[^3]: [OAuth](https://en.wikipedia.org/wiki/OAuth) is a secure way of authenticating users in online applications.

[^4]: Tested services are: Bazqux, FreshRSS, Inoreader, Miniflux, Reedah, TheOldReader.

[^5]: XMPP plugin is only available for Qt 6 (`qt6`) builds.