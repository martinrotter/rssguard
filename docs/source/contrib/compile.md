Compiling RSS Guard
===================
RSS Guard is a `C++` application. All common build instructions can be found at the top of [CMakeLists.txt](https://github.com/martinrotter/rssguard/blob/master/CMakeLists.txt).

If you want a practical reference for the currently maintained CI builds, look at `resources/scripts/github-actions/`. Those scripts show how the official Windows, Linux, and macOS packages are assembled in GitHub Actions.

## Windows Installer
The Windows build stages its runtime files in `rssguard-build/app`. Before running NSIS, `package-windows.ps1` calls `prepare-windows-installer.py` to validate the staged executable, core DLL, extractor, Qt platform plugin, and service plugins. Use Python 3 on `PATH` (CI uses Python 3.13). The helper rejects staged user data and links, and generates `NSIS.payload.nsh` with the exact core payload and uninstall paths. Debug symbols (`.pdb`) and MSVC import libraries (`.lib`) are kept in the portable archive rather than the installer.

To regenerate that include when packaging manually, run this from the build directory:

```text
python <source>/resources/scripts/github-actions/prepare-windows-installer.py --stage app --definitions NSIS.definitions.nsh --output NSIS.payload.nsh
<source>/resources/scripts/nsis/makensis.exe /NOCONFIG /WX "/XOutFile rssguard-installer.exe" NSIS.template.in
```

Keep `nsProcess.nsh` and the Unicode `nsProcess.dll` when updating the bundled NSIS toolchain. CMake tracks edits to the installer template and refreshes the build copy automatically. The installer creates `.rssguard-install.ini` and `Uninstall.exe` for recovery before extracting the application; neither file belongs in the staged package. The record identifies the product, format, and normalized installation directory independently of the shared Installed Apps entry, and is retained until payload removal succeeds.

Run `python -m unittest discover -s resources/scripts/github-actions/tests -p test_windows_installer.py -v` to exercise payload validation. On Windows with the bundled NSIS available, the same suite compiles the actual template and tests installation, upgrades, failure/retry sequences, and removal using temporary payloads, private shortcut directories, and a unique per-user registry key. Legacy upgrade tests use the frozen pre-hardening installer in `tests/fixtures/NSIS.previous.nsi`, rather than relabeling the new template. It does not install the real application or require elevation. With CMake and Ninja available it also checks incremental template regeneration.

## Article And Web Viewer Backend
RSS Guard can be built with one of two article/web viewer backends:
* `WEB_ARTICLE_VIEWER_WEBENGINE=ON` - builds the Qt WebEngine-based viewer. This is the most browser-like and feature-complete option.
* `WEB_ARTICLE_VIEWER_WEBENGINE=OFF` - builds the QTextBrowser-based viewer. This is lighter and simpler, but it intentionally lacks some browser features.

Official CI binaries include the selected viewer type in their file names:
* `web` means the Qt WebEngine viewer is used.
* `text` means the QTextBrowser viewer is used.

For example, a Windows package name can contain `web-qt6-win10` or `text-qt6-win10`, and a Linux AppImage can contain `web-qt6-linux64` or `text-qt6-linux64`.

## Official macOS Packages
CI builds both Qt 6 viewers natively on `macos-15-intel` (`x86_64`, suffix `mac64intel`) and `macos-15` (`arm64`, suffix `mac64arm`). The shared build script takes `MACOS_ARCHITECTURE` and `MACOS_PACKAGE_SUFFIX` from the matrix and rejects mismatched runners. It sets the CMake architecture and the Go helper's `GOARCH` separately.

The deployment target is macOS 13.0. ICU is located using `brew --prefix icu4c`; if its installed libraries require a newer macOS or have the wrong architecture, CI builds Homebrew's verified ICU source with the package deployment target. The Qt SDK's CPU slices are checked before compilation.

`validate-macos-package.py` mounts the finished DMG and checks its metadata, required binaries, SQLite driver, CPU slices, dependency paths, minimum OS requirements and code signatures. It runs the shipped application and extractor, then uses a separate CI executable to exercise real application startup and article rendering against a copy of the packaged libraries. SDK library/plugin overrides are removed for these tests. The CI executable is not included in release packages.

```{warning}
Note that on macOS, in some cases, you have to self-sign the application via the `codesign` utility to make it run.
```
