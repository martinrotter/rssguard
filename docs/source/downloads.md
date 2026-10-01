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
Official package names identify the article/web viewer:
* `web` - uses Qt WebEngine for browser-style HTML and CSS rendering.
* `text` - uses the smaller, simpler QTextBrowser viewer, with limited website support and no JavaScript.

Choose `web` for the most complete rendering, or `text` for a lighter package. See [Displaying Articles And Web Pages](features/article-display.md) for a detailed comparison.

```{note}
Windows package names also identify the supported system:
* `win7` - intended for maximum compatibility, including older Windows systems. It is also fine to use on newer Windows versions.
* `win10` - intended for modern Windows systems. It includes the newer Qt 6-based build.

If you are unsure which one to choose, start with the `web` + `win10` package on Windows 10 or newer. Use `text` if you prefer a lighter build or you have RSS Guard crashing, and use `win7` when compatibility with older Windows matters more. See more [here](features/article-display).
```

### Windows
On Windows, there are several ways to install RSS Guard:
* Portable-style `7z` [packages](https://github.com/martinrotter/rssguard/releases). Simply download and unpack them.
* [Installers](https://github.com/martinrotter/rssguard/releases) created with [NSIS](https://nsis.sourceforge.io/Main_Page). These are self-contained and are the recommended option for most users. They support normal upgrades and clean uninstallation, while still letting you keep your data if you want.

```{warning}
RSS Guard binaries provided via the official [download page](https://github.com/martinrotter/rssguard/releases) are not digitally signed. Because of that, Microsoft Defender SmartScreen may show a warning the first time you launch the application.
```

```{attention}
Most required runtime files are bundled with official packages, but if Windows still reports a missing runtime DLL, installing the current Microsoft Visual C++ redistributables usually fixes it. If you want a broader package, this [All-in-One runtime DLL pack](https://github.com/abbodi1406/vcredist/releases) is also a practical option.

Also, if your RSS Guard crashes on Windows upon article selection or when starting, then you might have some DirectX libraries missing. Install them with the below command or use dedicated DirectX installers from Microsoft. You can see more information about the crashes if you run RSS Guard with file [logging](contrib/bugs.md#debug-log) enabled.

`dism /Online /Add-Capability /CapabilityName:Tools.Graphics.DirectX~~~~0.0.1.0`
```

### Linux
The best option is to use official distribution packages if your distribution offers them.

If you prefer a standalone package, official release assets provide an `AppImage`. AppImage names also include either `web` or `text`, depending on the bundled article/web viewer.

### macOS
Official Qt 6 `DMG` packages are provided for macOS 13 or newer, with separate builds for Intel and Apple Silicon. Choose the suffix matching your Mac's processor:

| Package suffix | Mac processor | Available viewers |
| --- | --- | --- |
| `mac64intel.dmg` | Intel (`x86_64`) | `web` and `text` |
| `mac64arm.dmg` | Apple Silicon (`arm64`, M-series chips) | `web` and `text` |

For example, `rssguard-<version>-web-qt6-mac64intel.dmg` is the full web viewer for Intel Macs, while `rssguard-<version>-text-qt6-mac64arm.dmg` is the lighter text viewer for Apple Silicon. Development builds use the same suffixes.

You can find your processor in **Apple menu > About This Mac**: Intel Macs show a processor, and Apple Silicon Macs show an Apple M-series chip. Older releases named only `mac64` do not identify their architecture; the 5.2.5 `mac64` packages support Apple Silicon only.

```{warning}
Official macOS packages support the SQLite database backend only. They do not include the MariaDB/MySQL database driver, so existing MariaDB-based installations cannot use these packages.
```

```{warning}
On macOS, Gatekeeper may block unsigned applications. If that happens, you may need to remove the quarantine flag or self-sign the application with `codesign`.
```

```{attention}
CI checks each DMG's architecture, bundled dependencies and minimum macOS version, then tests startup, article rendering and extraction on a native Intel or Apple Silicon runner. Testing on older macOS versions and additional Mac hardware still depends on user reports because we do not have macOS-dedicated testers and maintainers.
```

### KOBO Book Reader
Yes, RSS Guard was [ported](https://github.com/Szybet/rssguard-inkbox) to KOBO. Feel free to try it and report any bugs upstream or to the port author.
