Database Backends
=================
RSS Guard offers switchable database backends to hold your data. At this point, two backends are available:
* MariaDB
* SQLite (default)

SQLite backend is very simple to use, no further configuration needed. All your data is stored in a single file:
```
<user-data-folder>\database\database.db
```
(For path to user data folder, see User Data Portability section.)

This backend offers an `in-memory` database option, which automatically copies all your data into RAM when application launches, making RSS Guard incredibly fast. Data is written back to database file on disk when application exits. This option is not expected to be used often because RSS Guard should be fast enough with classic SQLite persistent DB files. Use this option only with huge amount of article data, and when you know what you are doing.

Also note, that some new versions of RSS Guard introduce changes to how application data are stored in database file. When this change happens, backup of your SQLite database file is created automatically.

MariaDB (MySQL) backend is there for users who want to store their data in a centralized way. You can have a single server in your network and use multiple RSS Guard instances to access the data.

For database-related configuration see `Settings -> Data storage` dialog section.