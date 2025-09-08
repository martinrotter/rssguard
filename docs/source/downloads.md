Downloads & Installation
========================

## Downloads
Official place to download RSS Guard is at [Github Releases page](https://github.com/martinrotter/rssguard/releases). You can also download the [development (beta) build](https://github.com/martinrotter/rssguard/releases/tag/devbuild5), which is updated automatically every time the source code is updated (there is like 30 minutes delay because the compilation step takes some time).

RSS Guard is also available in [repositories of many Linux distributions](https://repology.org/project/rssguard/versions), and via [Flathub](https://flathub.org/apps/search?q=rssguard).

I highly recommend to download RSS Guard only from trusted sources.

## Installation
### Windows
On Windows, there are some ways of installing RSS Guard:
* Portable-style `7z` [packages](https://github.com/martinrotter/rssguard/releases). Simply download, unpack and that's it.
* [Installers](https://github.com/martinrotter/rssguard/releases) (created with [NSIS](https://nsis.sourceforge.io/Main_Page)). These are self-contained, you simply install and you are ready to go. Note that installer is written in very clean way and is meant to behave! When you uninstall with it, it should properly remove all traces (and asks if you want to keep your data). Also, it remembers where you installed RSS Guard so upgrades are easy. Using these installers is likely recommended way.
* Packages created with [Chocolatey](https://community.chocolatey.org/packages/rssguard). These are nice, but unofficial. They work allright.

```{note}
Official RSS Guard Windows packages come in two flavors:
* `win7` - built with Qt version 5, uses `QtMultimedia` for [media player](features/mediaplayer), smaller installation size, smaller memory footprint
* `win10` - built with Qt version 6, uses `libmpv` for media player (can play almost everything), bigger installation size
```

```{warning}
RSS Guard binaries provided via official [download page](https://github.com/martinrotter/rssguard/releases) are NOT digitally signed. This means that you can get Microsoft Defender SmartScreen popup dialog when launching RSS Guard for the first time, which warns you about risks related to using unsigned software. This is completely harmless.
```

```{attention}
Note that some MSVC runtime libraries are bundled with RSS Guard but sometimes you might get errors like some DLL is missing. If that happens I highly recommend you to install this great [All-in-One runtime DLL pack](https://github.com/abbodi1406/vcredist/releases).
```

### Linux
Best way is to use official distribution packages if your distribution offers those.

```{attention}
RSS Guard used to be available in `AppImage` format for Linux. The support for this format was dropped because `AppImage` lacks proper support and up-to-date build tools and has some undocumented quirks.

It is recommended to either use native packages for your distribution, compile by yourself or stick to `Flatpak` RSS Guard [packages](https://flathub.org/apps/search?q=rssguard).
```

### Mac OS X
`DMG` packages are provided for Mac users.

```{warning}
Note that on Mac OS X, in some cases, you have to self-sign the application via `codesign` utility to make it run.
```

### KOBO Book Reader
Yes, RSS Guard was [ported](https://github.com/Szybet/rssguard-inkbox) to KOBO. Feel free to try it and report any bugs upstream or to port author.