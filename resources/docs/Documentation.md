# Documentation
* [Introduction](#introduction)
    * [Downloads](Downloads.md)
    * [Philosophy](#philosophy)
    * [Versioning](#versioning)
    * [How to contribute](#how-to-contribute)
    * [Reporting bugs](#reporting-bugs)
    * [Localizations](#localizations)
    * [Videos](#videos)
    * [Web-based and lite app variants](#web-based-and-lite-app-variants)
    * [RSS Guard 3 vs. RSS Guard 4](#rss-guard-3-vs-rss-guard-4)
* [Features](#features)
    * [List of main features](#list-of-main-features)
    * [Supported feed formats and online feed services](Feed-formats.md)
    * [Message filtering](Message-filters.md)
    * [Database backends](#database-backends)
    * [Google Reader API](#google-reader-api)
    * [Websites scraping](#websites-scraping)
    * [Gmail](#gmail)
    * [Feedly](#feedly)
    * [Labels](Labels.md)
    * [Downloading files](#downloading-files)
    * [External tools](#external-tools)
    * [AdBlock](#adblock)
    * [GUI tweaking](#gui-tweaking)
* [Miscellaneous (for advanced users)](#miscellaneous-for-advanced-users)
    * [Command line interface](#cli)
    * [How to build](#how-to-build)
    * [`%data%` placeholder](#data-placeholder)
    * [Cleaning database](#cleaning-database)
    * [Portable user data](#portable-user-data)
    * [Downloading new messages](#downloading-new-messages)
    * [Generating debug log file](#generating-debug-log-file)

<img src="images/rssguard.png" width="64px">

# Introduction
First, let me say, that you can contact RSS Guard's lead developer via [e-mail](mailto:rotter.martinos@gmail.com) or just submit a ticket here in the repository.

I am glad to accept any kind of donations, see ♥ **Sponsor** button on the top of this page. **I say "thank you" for all your support, my donators.** Also, I personally send "thank you" to all contributors (translators, source code contributors, issue reporters) and users.

Note that some more elaborate parts of this documentation are placed in separate files for practical purposes.

## Philosophy
RSS Guard tends to be independent software. It's free, it's open-source.

## Versioning
RSS Guard uses [semantic versioning](https://semver.org/). The versioning scheme is `X.Y.Z`, where:

* `X` marks major release version. This number will change very rarely and indicates critical new changes breaking backward compatibility.
* `Y` indicates that there is new major feature available.
* `Z` indicates that there are newly fixed bugs or small features introduced.

## How to contribute
RSS Guard is open source application with free GNU GPLv3 license. Everyone willing to contribute is welcomed. You can contribute in many ways by:
* fixing and [reporting](#reporting-bugs) bugs and requesting PRs (pull requests),
* [localizing](#localizations) to other languages,
* supporting author with [donations](https://github.com/sponsors/martinrotter),
* spreading the word.

If you decide to contribute code, then please try to follow the style and formatting of existing source code. Also, I use [uncrustify](https://github.com/martinrotter/rssguard/blob/master/resources/scripts/uncrustify/uncrustify.cfg) to format source code.

## Reporting bugs
Please report all issues/bugs/ideas to [Issues](https://github.com/martinrotter/rssguard/issues) section. Describe your problem as precisely as possible, along with steps taken leading up to the issue occurring.

Also, for some broader questions or general ideas, use [discussions](https://github.com/martinrotter/rssguard/discussions) rather than [issues](https://github.com/martinrotter/rssguard/issues).

It is a good idea to read [this](http://www.chiark.greenend.org.uk/~sgtatham/bugs.html) before reporting the bug; it will save time and effort for everyone if all the required information is provided from the get-go.

## Localizations
RSS Guard currently includes [many localizations](http://www.transifex.com/projects/p/rssguard).

If you are interested in creating translations for RSS Guard, then do this:

1. Go [here](http://www.transifex.com/projects/p/rssguard) and check status of currently supported localizations.
2. [Login](http://www.transifex.com/signin) (you can use social networks to login) and work on existing translations. If no translation team for your country/language exists, then ask for creating of localization team via the website.

**All translators commit themselves to keep their translations up-to-date. If some translations are not updated by their authors regularly and only small number of strings is translated, then those translations along with their teams will be eventually REMOVED from the project!!! At least 50% of strings must be translated for translation to being added to project.**

## Videos
Here you can see some videos to see RSS Guard in action:
* [Short RSS Guard sample](videos/rssguard.gif).
* [Hiding GUI elements](videos/hiding-gui.gif).

## Web-based and lite app variants
RSS Guard is distributed in two variants:
* **Standard package with WebEngine-based bundled message viewer**: This variant displays messages with their full formatting and layout in embedded Chromium-based web viewer. This variant of RSS Guard should be nice for everyone who doesn't care about memory consumption too much. Also, installation packages are relatively big.

<img src="images/webengine-view.png" width="80%">

* **Lite package with simple text-based message viewer**: This variant displays message in much simpler and more lightweight text-based component. All packages of this variant have `nowebengine` keyword in their names. Layout and formatting of displayed message is simplified, no big external web viewers are used, which results in much smaller installation packages, much smaller memory footprint and increased privacy of the user, because many web resources are not downloaded by default like pictures, JavaScript and so on. This variant of RSS Guard is meant for advanced users and offers faster GUI response in some use-cases.

<img src="images/nonwebengine-view.png" width="80%">

If you're not sure which version to use, **use the WebEngine-based RSS Guard**.

## RSS Guard 3 vs. RSS Guard 4
RSS Guard 4 is **NOT** backwards compatible with previous editions of the application!!! It stores settings in slightly different [folder](#portable-user-data) to not overwrite user data from previous versions.

RSS Guard 4.x contains numerous enhancements and many of them are hidden under the hood and they make application easier to maintain, easier to improve and easier to use.

# Features
RSS Guard is simple (yet powerful) feed reader. It is able to fetch the most known feed formats, including RSS/RDF/ATOM/JSON. RSS Guard is developed on top of the [Qt library](http://qt-project.org) and it supports these operating systems:

* Windows,
* GNU/Linux,
* Mac OS X,
* OS/2 (ArcaOS, eComStation),
* Android (prebuilt binaries N/A at this point).

## List of main features
* **support for online feed synchronization via plugins**,
    * Tiny Tiny RSS,
    * Nextcloud News,
    * Inoreader,
    * Gmail.
    * Google Reader API (FreshRSS, The Old Reader, Bazqux, Reedah and others),
    * Feedly.
* core:
    * support for all feed formats (RSS/RDF/ATOM/JSON),
    * full support of podcasts (RSS/ATOM/JSON),
    * import/export of feeds to/from OPML 2.0,
    * universal plugin for online services with [Google Reader API](#google-reader-api),
    * possibility of using custom 3rd-party feed synchronization services,
    * feed metadata fetching including icons,
    * support for [scraping websites](#websites-scraping) which do not offer RSS/ATOM feeds and other related advanced features,
    * simple internal Chromium-based web viewer (or alternative version with simpler and much more lightweight internal viewer),
    * scriptable [message filtering](#message-filtering),
    * downloader with own tab and support for up to 6 parallel downloads,
    * application-wide and account-specific network proxies support,
    * ability to cleanup internal message database with various options,
    * enhanced feed auto-updating with separate time intervals,
    * "portable" mode support with clever auto-detection,
    * feed categorization,
    * feed authentication (BASIC),
    * [external tools](#external-tools) - you can run your program with article URL,
    * handles tons of messages & feeds,
    * ability to backup/restore database or settings,
    * fully-featured recycle bin,
    * multiple data backend support,
        * SQLite,
        * MariaDB.
    * ability to specify target database by its name (MariaDB backend),
    * support for `feed://` URI scheme.
* user interface:
    * message list filter with regular expressions,
    * drag-n-drop for feed list,
    * able to show unread feeds/messages only,
    * can be controlled via keyboard,
    * fully adjustable toolbars (changeable buttons and style),
    * hideable main menu, toolbars and list headers,
    * bundled icon themes (Numix & Papirus),
    * fully skinable user interface + ability to create your own skins,
    * newspaper view,
    * tabbed interface,
    * ability to hide list of feeds/categories,
    * desktop integration via tray icon,
    * localizations to some languages,
    * ability to tweak columns in displayed list of messages.

## Database backends
RSS Guard offers switchable database backends which hold your data. At this point, two backends are available:
* MariaDB,
* SQLite (default).

SQLite backend is very simple to use, no further configuration is needed and all your data is stored in single file
```
<user-data-root-path>\database\local\database.ini
```
Check `About RSS Guard -> Resources` dialog to find more info on significant paths used. This backend offers "in-memory" database option, which automatically copies all your data into RAM when app starts and then works solely with that RAM data, which makes RSS Guard incredibly fast. Data is also written back to database file when app exits. Note that this option should be used very rarely because RSS Guard should be fast enough with classic SQLite persistent DB files. So only use this if you know what you are doing.

MariaDB (MySQL) backend is there for users, who want to store their data in a centralized way. You can have single server in your network and use multiple RSS Guard instances to access the data. MySQL will also work much better if you prefer to have zillions of feeds and messages stored.

For database-related configuration see `Settings -> Data storage` dialog.

## Google Reader API
Starting with RSS Guard 3.9.0, there is a new plugin which offers synchronization with services using Google Reader API. Plugin was so far tested with FreshRSS, Reedah, The Old Reader and Bazqux. All Google Reader API enabled services should work.

Note that Inoreader has its own separate plugin, because it uses OAuth as authentication method, therefore it is cleaner to have separate plugin.

Google Reader API integration in RSS Guard offers a way to set custom service endpoint even if you select service which is not self-hosted such as Bazqux, providing all users with greater flexibility and freedom.

<img src="images/greader-api-settings.png">

Note that even when all Google Reader API enabled services should follow the API, there are still some minor differences, primarily because Google Reader API has no strict documentation to follow and some services do not offer some features.

For example The Old Reader does not seem to offer tags/labels functionality, therefore tags/labels in RSS Guard are not synchronized, but you can still use offline labels.

## Websites scraping

> **Only proceed if you consider yourself to be a power user and you know what you are doing!**

RSS Guard 3.9.0+ offers extra advanced features which are inspired by [Liferea](https://lzone.de/liferea/).

You can select source type of each feed. If you select `URL`, then RSS Guard simply downloads feed file from given location and behave like everyone would expect.

However, if you choose `Script` option, then you cannot provide URL of your feed and you rely on custom script to generate feed file and provide its contents to **standard output**. Resulting data written to standard output should be valid feed file, for example RSS or ATOM XML file.

`Fetch it now` button also works with `Script` option. Therefore, if your source script and (optional) post-process script in cooperation deliver a valid feed file to the output, then all important metadata, like title or icon of the feed, can be automagically discovered.

<img src="images/scrape-source-type.png" width="50%">

Any errors in your script must be written to **error output**.

Note that you must provide full execution line to your custom script, including interpreter binary path and name and all that must be written in special format `<interpreter>#<argument1>#<argument2>#....`. The `#` character is there to separate interpreter and individual arguments. I had to select some character as separator because simply using space ` ` is not that easy as it might sound, because sometimes space could be a part of an argument sometimes argument separator etc.

Used script must return `0` as process exit code if everything went well, or non-zero exit code if some error happened.

Interpreter must be provided in all cases, arguments do not have to be. For example `bash.exe#` is valid execution line, as well as `bash#-c#cat feed.atom`. Note the difference in interpreter's binary name suffix. Also be very carefully about arguments quoting. Some examples of valid and tested execution lines are:
 
| Command | Explanation |
|---------|-------------|
| `bash#-c#curl https://github.com/martinrotter.atom` | Downloads ATOM feed file with Bash and Curl. |
| `Powershell#Invoke-WebRequest 'https://github.com/martinrotter.atom' \| Select-Object -ExpandProperty Content` | Downloads ATOM feed file with Powershell. |
| `php#tweeper.php#-v#0#https://twitter.com/NSACareers` | Scrape Twitter RSS feed file with [Tweeper](https://git.ao2.it/tweeper.git). Tweeper is utility which is able to produce RSS feed from Twitter and other similar social platforms. |

<img src="images/scrape-source.png" width="50%">

Note that the above examples are cross-platform and you can use the exact same command on Windows, Linux or Mac OS X, if your operating system is properly configured.

RSS Guard offers [placeholder](#data-placeholder) `%data%` which is automatically replaced with full path to RSS Guard's [user data folder](Documentation.md#portable-user-data). You can, therefore, use something like this as source script line: `bash#%data%/scripts/download-feed.sh`.

Also, working directory of process executing the script is set to RSS Guard's user data folder.

There are some examples of website scrapers [here](https://github.com/martinrotter/rssguard/tree/master/resources/scripts/scrapers), most of the are written in Python 3, thus their execution line is `python#script.py`.

After your source feed data are downloaded either via URL or custom script, you can optionally post-process the data with one more custom script, which will take **raw source data as input** and must produce processed valid feed data to **standard output** while printing all error messages to **error output**.

Format of post-process script execution line is the same as above.

<img src="images/scrape-post.png" width="50%">

Typical post-processing filter might do things like advanced CSS formatting or filtering of feed file entries or removing ads:

| Command | Explanation |
|---------|-------------|
| `bash#-c#xmllint --format -` | Pretty-print input XML feed data. |

It's completely up to you if you decide to only use script as `Source` of the script or separate your custom functionality between `Source` script and `Post-process` script. Sometimes you might need different `Source` scripts for different online sources and the same `Post-process` script and vice versa.

## Gmail
RSS Guard includes Gmail plugin, which allows users to receive and send e-mail messages in a very simple fashion. Plugin uses [Gmail API](https://developers.google.com/gmail/api) and offers some e-mail client-like features:
* Sending e-mail messages.

<img src="images/gmail-new-email.png">

* You can also reply to existing messages.
* Plugin is able to suggest recipient's e-mail. Suggestable addresses are read from e-mail messages which are already stored in RSS Guard's database. Therefore you have to have some e-mails fetched in order to have this feature working.

## Feedly
RSS Guard 3.9.0+ offers [Feedly](https://feedly.com) plugin. Note that there are some specifics when using the plugin.

Some official builds of RSS Guard might include official Feedly support and will provide almost unlimited API quotas and ability to just login with your username and password.

Sadly, some builds of RSS Guard do not have embedded production Feedly API keys and thus no official support from Feedly, therefore you must use something called `developer access token` to be able to use the plugin. See below image and notice the `Get token` button which will lead you to Feedly authentication page where you can generate the token.

<img src="images/feedly-details.png">

There are two big downsides of using `developer access token`:
* It expires after one month and must be manually renewed.
* It allows maximum of 250 API calls per day.

## Downloading files
RSS Guard offers simple embedded file downloader.

<img src="images/downloader-window.png" width="80%">

You can right click on any item in embedded web browser and hit `Save as` button. RSS Guard will then automatically display downloader and will download your file. This feature works in both RSS Guard [variants](#web-based-and-lite-app-variants).

<img src="images/downloader-view.png" width="80%">

You can download up to 6 files simultaneously.

## External tools
RSS Guard allows you to define a set of custom tools which you can subsequently launch with article URL being passed as parameter to your tool's binary. The feature also supports passing direct links to various media source such as pictures or videos, when used with internal web browser's context menu.

<img src="images/ext-tools-message.png" width="80%">
<img src="images/ext-tools-web.png" width="80%">

## AdBlock
[Web-based variant](#web-based-and-lite-app-variants) of RSS Guard offers ad-blocking functionality. AdBlock uses standard AdBlock-Plus-like scripts, thus allowing you to use EasyList etc. AdBlock supports all fundamental features of AdBlock-Plus format, including element hiding rules and site-wide blocking.

You can find its settings in `Web browser & tabs` section of main menu. AdBlock is of course available only in [WebEngine-based](#web-based-and-lite-app-variants) version of the app.

## GUI tweaking
RSS Guard's GUI is very customizable. You can, for example, hide many GUI elements. There are even people who use RSS Guard on mobile devices powered by Linux like PinePhone or Librem devices.

<img src="images/gui-hiding.png" width="80%">

For example, you can hide menu, various toolbars, statusbar and even list headers to achieve very minimal main window layout.

If you hide main menu, then small `home` icon will appear in left-top corner of main application window. 

<img src="images/gui-hiding-all.png" width="80%">

Many people have very widescreen monitors nowadays and RSS Guard offers you horizontal layout for this use case, placing message previewer on the right side of message list.

<img src="images/gui-layout-orientation.png" width="80%">

# Miscellaneous (for advanced users)
Here you can find some useful advanced insights into RSS Guard's modus operandi.

## CLI
RSS Guard offers CLI (command line interface). For overview of its features, run `rssguard --help` in your terminal. You will see the overview of the interface.

```
RSS Guard

Options:
  -l, --log <log-file>           Write application debug log to file. Note that
                                 logging to file may slow application down.
  -d, --data <user-data-folder>  Use custom folder for user data and disable
                                 single instance application mode.
  -s, --no-single-instance       Allow running of multiple application
                                 instances.
  -?, -h, --help                 Displays help on commandline options.
  --help-all                     Displays help including Qt specific options.
  -v, --version                  Displays version information.
```

## How to build
RSS Guard is C++ application and all common build instructions can be found in top of [project file](https://github.com/martinrotter/rssguard/blob/master/build.pro).

### OS/2-specifics
RSS Guard can run on OS/2 and if you want to compile it by yourself, you need to make sure that your OS/2 distribution is up-to-date and you have all dependencies installed:
* `os2-base`,
* all `gcc-*` packages,
* `libc` and `libcx` up-to-date,
* `kbuild-make`,
* `ash`,
* `binutils`,
* all relevant `qt5-*` packages.

Make sure you really have all development dependencies installed as OS/2 is known to thro erratic errors if you miss some important compile-time dependency.

After your dependecies are installed, then you can compile via standard `qmake -> make -> make install` steps and package with

```
7z.exe a -t7z -mmt -mx9 "rssguard.7z" "<build-folder\src\rssguard\app\*"
```

command.

## `%data%` placeholder
RSS Guard stores its data and settings in single folder. What exact folder it is is described [here](#portable-user-data). RSS Guard allows you to use the folder programmatically in some special contexts via `%data%` placeholder. You can use this placeholder in these RSS Guard contexts:
* Contents of your [message filters](Message-filters.md) - you can therefore place some scripts under your user data folder and include it via `JavaScript` into your message filter.
* `source` and `post-process script` attributes of for [scraping](#websites-scraping) feed - you can use the placeholder to load scripts to generate/process feed from user data folder.

## Cleaning database
Your RSS Guard's database can grow really big over time, therefore you might need to do its cleanup regularly. There is a dialog `Cleanup database` in `Tools` menu to do just that for you, but note that RSS Guard should run just fine even with tens of thousands of messages.

<img src="images/cleanup-db.png">

## Portable user data
RSS Guard checks "config directory" (this is `C:\Users\<user>\AppData\Local` directory on Windows) for existence of file:
```
RSS Guard 4\data\config\config.ini
```
If that file exists, then RSS Guard will use the file (this is called _non-portable **FALLBACK** settings_). If this file is not found, then application will check if its root path (folder, in which RSS Guard executable is installed) is writable, and if it is, it will store settings in it, in subfolder:
```
data4\config\config.ini
```
This is _fully-portable mode_. Check `About RSS Guard -> Resources` dialog to find more info on significant paths used.

RSS Guard on Linux, Android or Mac OS automatically uses non-portable user data location, so that it is in line with [XDG](https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html) standard.

## Downloading new messages
Here is the rough workflow which is performed when you hit `Feeds & categories -> Update all items` or `Feeds & categories -> Update selected items`. At that point of time this happens:
1. RSS Guard creates a list of all/selected feeds.
2. Sequentially, for each feed do:
    * a. Download all available messages from online source or generate it with script.
    * b. Post-process messages with post-process script, if configured.
    * c. Sequentially, for each downloaded message, do:
        * 1. Sanitize title of the message. This includes replacing all non-breaking spaces with normal spaces, removing all leading spaces, replacing all multiple consecutive spaces with single space. Contents of message are converted from [percent-encoding](https://en.wikipedia.org/wiki/Percent-encoding).
        * 2. Run all [message filters](#message-filtering), one by one, one the message. Cache read/important message attributes (or labels changes) changed by filters to queue which is later synchronized back to online feed service.
        * 3. Store the message into RSS Guard's [database](#database-backends), creating completely new DB entry for it, or replacing existing message. **Note that two messages are considered as the same message if they have identical URL, author and title and they belong to the same feed.** This does not stand for synchronized feeds (TT-RSS, Inoreader and others) where each message has assigned special ID which identifies the message uniquely.

## Generating debug log file
If you run into problems with RSS Guard and you need your problems fixed, you should provide log file from the time when problem occurred. RSS Guard writes all important information to standard output, which is usually calling terminal.

To redirect debug output of RSS Guard to log file, do this:
1. Open command line.
2. Navigate to your RSS Guard installation folder.
3. Enter `rssguard.exe --log 'log.txt'`. RSS Guard will now start. You can of course specify arbitrary file where to store log and its location must be writable.
4. Now try to simulate your problem.
5. Attach generated `log.txt` file to your bug report.

On Windows, there are some problems if you want to see debug output of a GUI program, because Windows does not support "dual" applications. You can, however, display application's debug console output with PowerShell, specifically for RSS Guard like this: `.\rssguard.exe | Out-Default`. Just run this stuff when you are in RSS Guard's folder and you should see console output directly in your PowerShell window.
