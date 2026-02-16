Database Backends
=================
RSS Guard offers switchable database backends to hold your data. At this point, two backends are available:
* MariaDB
* SQLite (default)

SQLite backend is very simple to use, no further configuration needed. All your data is stored in a single file:
```
<user-data-folder>\database\database.db
```
(For path to user data folder, see [this](#userdata) section.)

Also note, that some new versions of RSS Guard introduce changes to how application data are stored in database file. When this change happens, backup of your SQLite database file is created automatically.

MariaDB (MySQL) backend is there for users who want to store their data in a centralized way. You can have a single server in your network and use multiple RSS Guard instances to access the data, but not simultaneously.

For database-related configuration see `Settings -> Data storage` dialog section.