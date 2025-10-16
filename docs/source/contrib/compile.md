Compiling RSS Guard
===================
RSS Guard is a `C++` application. All common build instructions can be found at the top of [CMakeLists.txt](https://github.com/martinrotter/rssguard/blob/master/CMakeLists.txt) and the [github build script's](https://github.com/martinrotter/rssguard/tree/master/resources/scripts/github-actions)

```{warning}
Note that on modern MacOS machines you will experience crashs when launching RSS Guard, and you will have to self-sign the application via `codesign` utility to make it run.

You can generally solve this this by executing this command in a terminal relative to the .app's installed location. 
```bash
codesign --deep -fs - "RSS Guard.app"
```

Here's a quick example of how to build it on Linux:

```bash
#Download from git
git clone --recurse-submodules https://github.com/martinrotter/rssguard
cd rssguard
# Create a build directory
mkdir rssguard-build
cd rssguard-build

# Configure the project to build using Qt 6, and disable built-in web browser support
cmake .. --warn-uninitialized -G Ninja -DFORCE_BUNDLE_ICONS="ON" -DCMAKE_VERBOSE_MAKEFILE="ON" -DREVISION_FROM_GIT="ON" -DBUILD_WITH_QT6="ON" -DENABLE_COMPRESSED_SITEMAP="ON" -DENABLE_MEDIAPLAYER_LIBMPV="ON" -DENABLE_MEDIAPLAYER_QTMULTIMEDIA="OFF"

# Compile it
cmake --build .

# (Optional) Run the build to test it
./src/rssguard/rssguard

# (Optional) Install RSS Guard system-wide
sudo cmake --install . --prefix /usr --verbose
```
