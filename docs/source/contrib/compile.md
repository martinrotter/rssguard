Compiling RSS Guard
===================
RSS Guard is a `C++` application. All common build instructions can be found at the top of [CMakeLists.txt](https://github.com/martinrotter/rssguard/blob/master/CMakeLists.txt).

If you want a practical reference for the currently maintained CI builds, look at `resources/scripts/github-actions/`. Those scripts show how the official Windows, Linux, and macOS packages are assembled in GitHub Actions.

```{warning}
Note that on macOS, in some cases, you have to self-sign the application via the `codesign` utility to make it run.
```
