Supported Feed Readers
======================
RSS Guard is a multi-account application. It can store [standard feeds](services/standard) locally or synchronize with supported online feed readers through service plugins.

When it comes to online web-based feed readers, these are supported:

| Service | Two-way Synchronization | Intelligent Synchronization Algorithm (ISA) [^1] | Synchronized Labels [^2] | OAuth [^3] |
| :---              | :---:  | :---: | :---: | :---: |
| [Feedly](services/feedly) | Yes | Yes | Yes | Yes (only for official binaries) |
| [Gmail](services/gmail) | Yes | Yes | Yes | Yes |
| [Google Reader API](services/google-reader-api) [^4] | Yes | Yes | Yes | Yes (only for Inoreader) |
| [Nextcloud News](services/nextcloud-news) | Yes | No | No | No |
| [Tiny Tiny RSS](services/tiny-tiny-rss) | Yes | Yes | Yes | No |

[^1]: Some plugins support the next-generation intelligent synchronization algorithm (ISA), which has several benefits. It usually offers better synchronization speed and transfers much less data over your network connection.

    With ISA, RSS Guard only downloads articles that are new or were updated by the remote server. The old algorithm usually fetches all available articles, even if they are not needed, which leads to unnecessary load on both your network connection and RSS Guard.

[^2]: Note that [labels](#features/labels) are supported for all plugins, but for some plugins they are local-only and are not synchronized with the service, usually because the service itself does not support the feature.

[^3]: [OAuth](https://en.wikipedia.org/wiki/OAuth) is a secure way of authenticating users in online applications.

[^4]: Tested services are: Bazqux, FreshRSS, Inoreader, Miniflux, Reedah, TheOldReader.

The **Supported Services** section of this documentation contains setup instructions and a description of each plugin.
