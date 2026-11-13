Plugin API
==========
A simple `C++` API allows you to create new service plugins. All base API classes can be found in the [`abstract`](https://github.com/martinrotter/rssguard/tree/master/src/librssguard/services/abstract) folder. The user must declare a subclass and implement all interface classes:

| Class                 | Purpose   |
| :---                  | ---       |
| `ServiceEntryPoint`   | The base class which provides basic information about the plugin name, author, etc. It also provides methods which are called when a new account is created or when existing accounts are loaded from the database. |
| `ServiceRoot`         | This is the core "account" class that represents an account node in the feed list and offers an interface for all critical plugin functionality, including handlers called on plugin start and stop, marking messages as read, unread, starred, or deleted, unassigning labels, and so on. |

The API is reasonably simple to understand, but relatively extensive. Sane defaults are used where it makes sense.

A plugin must be compiled as a separate library, which can then be loaded dynamically by RSS Guard. A good example of a plugin can be seen [here](https://github.com/martinrotter/rssguard/tree/devbuild5/src/librssguard-greader).

## Binary compatibility

Plugin JSON metadata must include integer `rssguardPluginApiVersion` matching `RSSGUARD_PLUGIN_API_VERSION` from `definitions.h`. RSS Guard checks this before constructing a plugin. Rebuild plugins when this version changes; new virtual methods change binary compatibility. The metadata IID remains provider-specific.

## Optional article columns

`ServiceRoot::articleColumns()` defaults to an empty list. Each `ArticleColumn` has a stable provider-local ID and translated title. Leave its built-in column index at `-1` to append a read-only, non-sortable column, or identify a built-in column to override its presentation. IDs and built-in targets must be unique; IDs must remain stable across translations and releases.

`articleColumnData(message, id, role)` supplies display, edit/raw and tooltip values from the loaded message. Keep this path inexpensive: painting must not make network calls or database queries. A built-in override retains its original raw edit value, SQL sorting and date-filter semantics. Extra columns participate in the existing column menu, all-column text search and numeric copy placeholders; title-only search still searches titles. Layouts use stable provider/column IDs and the existing source profiles. Providers with no overrides retain their existing layout keys.

## Managed message caches

`ServiceRoot::messageStoragePolicy()` defaults to existing article behavior. Providers can opt into metadata-only change detection, preservation of local read/important/trash/retrieval fields during normal remote updates, and exclusion from ordinary article cleanup. Explicit filter overwrites and permanent-deletion protection retain their existing semantics.

The cleanup preference is also persisted in account custom data so it remains effective if the plugin is unavailable. Targeted custom-data changes should use `DatabaseQueries::loadAccountCustomData()` / `modifyAccountCustomData()` inside a serialized write job, preserving other namespaces and current runtime tokens. Avoid nested synchronous database-worker calls.
