Plugin API
==========
A simple `C++` API allows creating new service plugins. All base API classes can be found in the [`abstract`](https://github.com/martinrotter/rssguard/tree/master/src/librssguard/services/abstract) folder. User must declare a subclass and implement all interface classes:

| Class                 | Purpose   |
| :---                  | ---       |
| `ServiceEntryPoint`   | The base class which provides basic information about the plugin name, author, etc. It also provides methods which are called when new account is created or when existing accounts are loaded from database. |
| `ServiceRoot`         | This is the core "account" class which represents an account node in feed's list, and offers interface for all critical functionality of a plugin, including handlers which are called with plugin's start/stop, marking messages as read/unread/starred/deleted, unassigning labels, etc. |

API is reasonably simple to understand but relatively extensive. Sane defaults are used where it makes sense.

Plugin must be compiled as separate library, which can then be dynamically loaded by RSS Guard. Good example of a plugin can be seen [here](https://github.com/martinrotter/rssguard/tree/devbuild5/src/librssguard-greader).
