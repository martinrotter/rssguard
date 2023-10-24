Supported Feed Readers
======================
RSS Guard is multi-account application and supports many web-based feed readers via built-in plugins. Its `standard` plugin provides support for these feed formats:
* RSS (2.0, 2.0.1)
* ATOM
* RDF
* [JSON](https://www.jsonfeed.org)
* [Sitemap](https://en.wikipedia.org/wiki/Sitemaps) (including Sitemap index discovery and compressed sitemaps)

When it comes to online web-based feed readers, these are supported:

| Service | Two-way Synchronization | Intelligent Synchronization Algorithm (ISA) [^1] | Synchronized Labels [^2] | OAuth [^3] |
| :---              | :---:  | :---: | :---: | :---:
| Feedly            | ✅ | ✅ | ✅ | ✅ (only for official binaries)
| Gmail             | ✅ | ✅ | ✅ | ✅
| Google Reader API [^4] | ✅ | ✅ | ✅ | ✅ (only for Inoreader)
| Nextcloud News    | ✅ | ❌ | ❌ | ❌
| Tiny Tiny RSS     | ✅ | ✅ | ✅ | ❌

[^1]: Some plugins support next-gen intelligent synchronization algorithm (ISA) which has some benefits, as it usually offers superior synchronization speed, and transfers much less data over your network connection.

    With ISA, RSS Guard only downloads articles which are new or were updated by remote server. The old algorithm usually always fetches all available articles, even if they are not needed, which leads to unnecessary overload of your network connection and the RSS Guard.

[^2]: Note that [labels](#features/labels) are supported for all plugins, but for some plugins they are local-only, and are not synchronized with the service. Usually because service itself does not support the feature.

[^3]: [OAuth](https://en.wikipedia.org/wiki/OAuth) is a secure way of authenticating users in online applications.

[^4]: Tested services are: Bazqux, FreshRSS, Inoreader, Miniflux, Reedah, TheOldReader.