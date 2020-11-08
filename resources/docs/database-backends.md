# Database Backends

RSS Guard has two database engines available for data storage

- [SQLite](https://www.sqlite.org/index.html)
- [MariaDB](https://mariadb.org/) (MySQL)

You can manually choose which backend RSS Guard will use by changing the settings manually:

1. Go to **Tools → Settings**
2. Select **Data Storage**

You have the option of 

## SQLite

The SQLite database engine is the one enabled by default as it provides a solid, proven and reliable way of storing database information that is easy to backup manually or via the use of file sync applications like Nextcloud, Dropbox, iCloud or OneDrive.

File locations:

- Windows, installed: `%%APPDATA%\database\local\database.db`
- Windows, portable: `.\data\database\local\database.db`
- Linux: `~/.config/RSS Guard/database/local/database.db`
- MacOS: 

Enabling "Use in-memory database as the working database" will copy the entire database file into system RAM at application start and uses that as the _working database_ for as long as RSS Guard is running; this makes the application incredibly fast. When you exit RSS Guard the file is copied from RAM back onto disk. There are a few caveats however:

- If your system loses power, you lose the working database entirely
- If RSS Guard crashes, you lose the working database entirely
- If you consistently handle hundreds of feeds, resulting in thousands of feed entries, the database file _will_ become extremely large. If your system is low on RAM you might experience issues not just with RSS Guard but on the entire system.

If you are using a solid state drive as main disk storage, RSS Guard should be fast enough for everyday heavy use.

## MariaDB

This database backend offers more power if you handle hundreds of feeds resulting in thousands of feed entries. To make use of this back end you _must_ have a server configured and running ahead of time. This configuration assumes the MariaDB server is either running in localhost or on a trusted local network and ALL PRIVILEGES have been given to the database user.

1. Go to **Tools → Settings**
2. Select **Data Storage**
3. Change **Database driver** to "MySQL/MariaDB (dedicated database) and fill out required settings:
    - Hostname
    - Port
    - Working database
    - Username
    - Password
4. **Test setup** to ensure everything is in working order
5. Click **OK** to save current settings.

This method offloads the manipulation of the database to a specialized server _but_ please remember the resources available to the database server, your connection bandwidth and your latency to the database server will have an strong effect on the everyday usage experience.

----

**Be aware, RSS Guard will transmit all data with no encryption. Only use this method if you are connecting to localhost or in a trusted local network.**

----

If you wish to use this functionality across the Internet it is **strongly** recommended you use one of the following options:

- Set up a VPN.
- Create an SSH tunnel. An example using OpenSSH would be something like `ssh -L 63306:database.server.example:3306 rssguard@database.server.example`, which will forward all traffic on localhost on port 63306 to `database.server.example` on port 3306.

Weighing these options and setting them up is an exercise left to the reader.


