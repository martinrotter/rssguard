Downloads & Installation
========================

## Downloads
Official place to download RSS Guard is at [Github Releases page](https://github.com/martinrotter/rssguard/releases). You can also download the [development (beta) build](https://github.com/martinrotter/rssguard/releases/tag/devbuild4), which is updated automatically every time the source code is updated (there is like 30 minutes delay because the compilation step takes some time).

RSS Guard is also available in [repositories of many Linux distributions](https://repology.org/project/rssguard/versions), and via [Flathub](https://flathub.org/apps/search?q=rssguard).

The are two different [flavors](#features/browseradblock):
* Regular: Includes an (almost) full-blown integrated web browser (built with `-NO_LITE=ON`).
* Lite: Includes simpler, safer (and less memory hungry integrated web browser (built with `-NO_LITE=OFF`).

I highly recommend to download RSS Guard only from trusted sources.

## Installation
### Windows
On Windows, there are some ways of installing RSS Guard:
* Portable-style `7z` [packages](https://github.com/martinrotter/rssguard/releases). Simply download, unpack and that's it.
* Installer (created with [NSIS](https://nsis.sourceforge.io/Main_Page)). These are self-contained, you simply install and you are ready to go. Note that installer is written in very clean way and is meant to behave! When you uninstall with it, it should properly remove all traces (and asks if you want to keep your data). Also, it remembers where you installed RSS Guard so upgrades are easy. Using these installers is likely recommended way.
* Packages created with [Chocolatey](https://community.chocolatey.org/packages/rssguard). These are nice, but unofficial. They work allright.

```{attention}
Note that some MSVC runtime libraries are bundled with RSS Guard but sometimes you might get errors like some DLL is missing. If that happens I highly recommend you to install this great [All-in-One runtime DLL pack](https://github.com/abbodi1406/vcredist/releases).

Simply download and launch newest available file `VisualCppRedist_AIO_x86_x64.exe`. It will install all up-to-date Microsoft libraries. It works great.
```

### Linux
Best way is to use official distribution packages if your distribution offers those.

```{attention}
RSS Guard used to be available in `AppImage` format for Linux. The support for this format was dropped because `AppImage` lacks proper support and up-to-date build tools and has some undocumented quirks.

It is recommended to either use native packages for your distribution, compile by yourself or sticking to `Flatpak` RSS Guard packages.
```

### Mac OS X
`DMG` packages are provided for Mac users.

### KOBO Book Reader
Yes, RSS Guard was [ported](https://github.com/Szybet/rssguard-inkbox) to KOBO. Feel free to try it and report any bugs upstream or to port author.