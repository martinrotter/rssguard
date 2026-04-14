Plugin API
==========
A simple `C++` API allows you to create new service plugins. All base API classes can be found in the [`abstract`](https://github.com/martinrotter/rssguard/tree/master/src/librssguard/services/abstract) folder. The user must declare a subclass and implement all interface classes:

| Class                 | Purpose   |
| :---                  | ---       |
| `ServiceEntryPoint`   | The base class which provides basic information about the plugin name, author, etc. It also provides methods which are called when a new account is created or when existing accounts are loaded from the database. |
| `ServiceRoot`         | This is the core "account" class that represents an account node in the feed list and offers an interface for all critical plugin functionality, including handlers called on plugin start and stop, marking messages as read, unread, starred, or deleted, unassigning labels, and so on. |

The API is reasonably simple to understand, but relatively extensive. Sane defaults are used where it makes sense.

A plugin must be compiled as a separate library, which can then be loaded dynamically by RSS Guard. A good example of a plugin can be seen [here](https://github.com/martinrotter/rssguard/tree/devbuild5/src/librssguard-greader).
