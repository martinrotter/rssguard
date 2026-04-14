Downloads & Installation
========================

## Downloads
The official place to download RSS Guard is the [GitHub Releases page](https://github.com/martinrotter/rssguard/releases). You can also download the [development (beta) build](https://github.com/martinrotter/rssguard/releases/tag/devbuild5), which is updated automatically every time the source code changes. There is roughly a 30-minute delay because the compilation step takes some time.

RSS Guard is also available in [repositories of many Linux distributions](https://repology.org/project/rssguard/versions), and via [Flathub](https://flathub.org/apps/search?q=rssguard).

I highly recommend downloading RSS Guard only from trusted sources.

## Installation
### Windows
On Windows, there are several ways to install RSS Guard:
* Portable-style `7z` [packages](https://github.com/martinrotter/rssguard/releases). Simply download and unpack them.
* [Installers](https://github.com/martinrotter/rssguard/releases) created with [NSIS](https://nsis.sourceforge.io/Main_Page). These are self-contained. You simply install the application and it is ready to use. Note that the installer is written in a clean way and is meant to behave properly. When you uninstall with it, it should remove all traces correctly, while also asking whether you want to keep your data. It also remembers where you installed RSS Guard, so upgrades are easy. Using these installers is the recommended option.
* Packages created with [Chocolatey](https://community.chocolatey.org/packages/rssguard). These are nice, but unofficial. They work all right.

```{note}
Official RSS Guard Windows packages come in two flavors:
* `win7` - built with Qt version 5, uses `QtMultimedia` for the [media player](features/mediaplayer), has a smaller installation size, and a smaller memory footprint. You can use this flavor even on newer Windows without any problems.
* `win10` - built with Qt version 6, uses `libmpv` for the media player and can play almost everything, but has a bigger installation size.
```

```{warning}
RSS Guard binaries provided via the official [download page](https://github.com/martinrotter/rssguard/releases) are NOT digitally signed. This means that you can get a Microsoft Defender SmartScreen popup dialog when launching RSS Guard for the first time, which warns you about risks related to using unsigned software. This is completely harmless.
```

```{attention}
Note that some MSVC runtime libraries are bundled with RSS Guard, but you might still see errors saying that a DLL is missing. If that happens, I highly recommend installing this [All-in-One runtime DLL pack](https://github.com/abbodi1406/vcredist/releases).
```

### Linux
The best option is to use official distribution packages if your distribution offers them.

### Mac OS X
`DMG` packages are provided for Mac users.

```{warning}
Note that on Mac OS X, in some cases, you have to self-sign the application via the `codesign` utility to make it run.
```

### KOBO Book Reader
Yes, RSS Guard was [ported](https://github.com/Szybet/rssguard-inkbox) to KOBO. Feel free to try it and report any bugs upstream or to the port author.
