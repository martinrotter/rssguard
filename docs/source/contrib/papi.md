Plugin API
==========
A simple `C++` API allows you to create new service plugins. All base API classes can be found in the [`abstract`](https://github.com/martinrotter/rssguard/tree/master/src/librssguard/services/abstract) folder. The user must declare a subclass and implement all interface classes:

| Class                 | Purpose   |
| :---                  | ---       |
| `ServiceEntryPoint`   | The base class which provides basic information about the plugin name, author, etc. It also provides methods which are called when a new account is created or when existing accounts are loaded from the database. |
| `ServiceRoot`         | This is the core "account" class that represents an account node in the feed list and offers an interface for all critical plugin functionality, including handlers called on plugin start and stop, marking messages as read, unread, starred, or deleted, unassigning labels, and so on. |

The API is reasonably simple to understand, but relatively extensive. Sane defaults are used where it makes sense.

A plugin must be compiled as a separate library, which can then be loaded dynamically by RSS Guard. A good example of a plugin can be seen [here](https://github.com/martinrotter/rssguard/tree/devbuild5/src/librssguard-greader).

Service plugin metadata must include an integer `rssguardPluginApiVersion` matching
`RSSGUARD_PLUGIN_API_VERSION` in `definitions.h`. RSS Guard checks it before constructing a plugin.
Rebuild external plugins for version 3. Services can override `ServiceRoot::alwaysDetectsArticleContentChanges()`
to compare article contents even when **Ignore contents changes** is enabled.

In version 2, `TabContent::webBrowser()` started returning the active browser
or `nullptr`. Interactive scroll commands check visibility. Version 2 removes
`scrollableWebBrowser()` and the empty `WebViewer::reloadNetworkSettings()` hook and adds
opt-in PKCE support to `OAuth2Service`.
