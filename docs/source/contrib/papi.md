Plugin API
==========
A simple `C++` API allows creating new service plugins. All base API classes can be found in the [`abstract`](https://github.com/martinrotter/rssguard/tree/master/src/librssguard/services/abstract) folder. User must declare a subclass and implement all interface classes:

| Class                 | Purpose   |
| :---                  | ---       |
| `ServiceEntryPoint`   | The base class which provides basic information about the plugin name, author, etc. It also provides methods which are called when new account is created or when existing accounts are loaded from database. |
| `ServiceRoot`         | This is the core "account" class which represents an account node in feed's list, and offers interface for all critical functionality of a plugin, including handlers which are called with plugin's start/stop, marking messages as read/unread/starred/deleted, unassigning labels, etc. |

API is reasonably simple to understand but relatively extensive. Sane defaults are used where it makes sense.

Perhaps the best approach to writing a new plugin is to copy the [existing one](https://github.com/martinrotter/rssguard/tree/master/src/librssguard/services/greader), and start from there.

Note that RSS Guard can support loading of plugins from external libraries (`.dll`, `.so`, etc.) but the functionality must be polished. At the moment, all plugins are directly bundled with the application, as no one really requested a run-time loading of plugins so far.