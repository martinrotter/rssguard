Database Backends
=================
RSS Guard offers switchable database backends to hold your data. At this point, two backends are available:
* MariaDB
* SQLite (default)

The SQLite backend is very simple to use, and no further configuration is needed. All your data is stored in a single file:
```
<user-data-folder>\database\database.db
```
For the path to the user-data folder, see [this](userdata) section.

Also note that some new versions of RSS Guard introduce changes to how application data is stored in the database file. When this happens, a backup of your SQLite database file is created automatically.

The MariaDB (MySQL) backend is intended for users who want to store their data centrally. You can have a single server on your network and use multiple RSS Guard instances to access the data, but not simultaneously.

```{warning}
The official macOS packages do not include the MariaDB/MySQL database driver and therefore support SQLite only. MariaDB remains available on platforms and in custom builds that provide a compatible Qt MySQL driver.
```

For database-related configuration, see the `Settings -> Data storage` dialog section.
