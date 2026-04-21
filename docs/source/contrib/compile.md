Compiling RSS Guard
===================
RSS Guard is a `C++` application. All common build instructions can be found at the top of [CMakeLists.txt](https://github.com/martinrotter/rssguard/blob/master/CMakeLists.txt).

If you want a practical reference for the currently maintained CI builds, look at `resources/scripts/github-actions/`. Those scripts show how the official Windows, Linux, and macOS packages are assembled in GitHub Actions.

## Article And Web Viewer Backend
RSS Guard can be built with one of two article/web viewer backends:
* `WEB_ARTICLE_VIEWER_WEBENGINE=ON` - builds the Qt WebEngine-based viewer. This is the most browser-like and feature-complete option.
* `WEB_ARTICLE_VIEWER_WEBENGINE=OFF` - builds the QTextBrowser-based viewer. This is lighter and simpler, but it intentionally lacks some browser features.

Official CI binaries include the selected viewer type in their file names:
* `web` means the Qt WebEngine viewer is used.
* `text` means the QTextBrowser viewer is used.

For example, a Windows package name can contain `web-qt6-win10` or `text-qt6-win10`, and a Linux AppImage can contain `web-qt6-linux64` or `text-qt6-linux64`.

```{warning}
Note that on macOS, in some cases, you have to self-sign the application via the `codesign` utility to make it run.
```
