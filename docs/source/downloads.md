Downloads & Installation
========================

## Downloads
The official place to download RSS Guard is the [GitHub Releases page](https://github.com/martinrotter/rssguard/releases).

If you want the newest testing build, use the [development (beta) build](https://github.com/martinrotter/rssguard/releases/tag/devbuild5). It is produced automatically by the CI pipeline, so new packages usually appear after the build finishes rather than instantly after each source-code change.

RSS Guard is also available in [repositories of many Linux distributions](https://repology.org/project/rssguard/versions), and via [Flathub](https://flathub.org/apps/search?q=rssguard).

I highly recommend downloading RSS Guard only from trusted sources.

```{warning}
There is `rssguard.com` website which is completely unrelated to this project and is NOT official place to download RSS Guard binaries. Be very careful!
```

## Installation
### Windows
On Windows, there are several ways to install RSS Guard:
* Portable-style `7z` [packages](https://github.com/martinrotter/rssguard/releases). Simply download and unpack them.
* [Installers](https://github.com/martinrotter/rssguard/releases) created with [NSIS](https://nsis.sourceforge.io/Main_Page). These are self-contained and are the recommended option for most users. They support normal upgrades and clean uninstallation, while still letting you keep your data if you want.
* Packages created with [Chocolatey](https://community.chocolatey.org/packages/rssguard). These are convenient, but unofficial.

```{note}
Official RSS Guard Windows packages come in two flavors:
* `win7` - intended for maximum compatibility, including older Windows systems. It is also fine to use on newer Windows versions.
* `win10` - intended for modern Windows systems. It includes the newer Qt 6-based build and is the better choice if you want the most feature-complete current Windows package.

If you are unsure which one to choose, start with `win10` on Windows 10 or newer, and use `win7` when compatibility matters more.
```

```{warning}
RSS Guard binaries provided via the official [download page](https://github.com/martinrotter/rssguard/releases) are not digitally signed. Because of that, Microsoft Defender SmartScreen may show a warning the first time you launch the application.
```

```{attention}
Most required runtime files are bundled with official packages, but if Windows still reports a missing runtime DLL, installing the current Microsoft Visual C++ redistributables usually fixes it. If you want a broader package, this [All-in-One runtime DLL pack](https://github.com/abbodi1406/vcredist/releases) is also a practical option.
```

### Linux
The best option is to use official distribution packages if your distribution offers them.

If you prefer a standalone package, official release assets provide an `AppImage`.

### macOS
`DMG` packages are provided for Mac users.

```{warning}
On macOS, Gatekeeper may block unsigned applications. If that happens, you may need to remove the quarantine flag or self-sign the application with `codesign`.
```

### KOBO Book Reader
Yes, RSS Guard was [ported](https://github.com/Szybet/rssguard-inkbox) to KOBO. Feel free to try it and report any bugs upstream or to the port author.
