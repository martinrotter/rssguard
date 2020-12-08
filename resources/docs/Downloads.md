# Downloads
See here to know where to download prebuilt versions of RSS Guard.

## Official downloads
Official downloads are available [here](https://github.com/martinrotter/rssguard/releases). Windows `exe/7zip` packages are published automatically when new RSS Guard version is released. Also `AppImage` packages for Linux and `dmg` packages for Mac OS X are automatically build.

[![RSS Guard in Instalki](http://www.instalki.pl/img/buttons/en/download_dark.png)](http://www.instalki.pl/programy/download/Windows/czytniki_RSS/RSS_Guard.html)

![RSS Guard is 100% clean.](http://www.softpedia.com/_img/softpedia_100_free.png)

## Development builds
Development builds can be downloaded [here](https://github.com/martinrotter/rssguard/releases/tag/devbuild).

## Installation packages naming
**Windows builds** of RSS Guard are generated automatically by the tool called AppVeyor. These builds have auto-generated names. In RSS Guard [downloads page](https://github.com/martinrotter/rssguard/releases) you can see filenames like:
 * `rssguard-3.4.2-7bad9d1-nowebengine-win32.7z`,
 * `rssguard-3.4.2-7bad9d1-win32.7z`,
 * `rssguard-3.4.2-95ee6be-nowebengine-win32.exe`,
 * `rssguard-3.4.2-95ee6be-win32.exe`.

The structure of these filenames is quite trivial and easily understandable for advanced users. For beginners, the overall structure of the file is `<projectname>-<version>-<commit>-<platform>.<fileformat>`. Example:
 * `<projectname>` = `rssguard` (This is self-explanatory.),
 * `<version>` = `3.4.2` (This describes the version of the application packaged in the file),
 * `<commit>` = `7bad9d1` (This describes the [Git commit](https://git-scm.com/docs/git-commit) used for the file. Whenever developers do some change to source code, that change gets assigned special ID, this is the ID.),
 * `<platform>` = `win32` (This is the target platform which the application can run on.),
 * `<fileformat>` = `exe` (This is self-explanatory.).

Note that same file naming scheme for development builds might be little different. Specifically, `<version>` field is omitted.