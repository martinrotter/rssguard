Compiling RSS Guard
===================
RSS Guard is a `C++` application. All common build instructions can be found at the top of [CMakeLists.txt](https://github.com/martinrotter/rssguard/blob/master/CMakeLists.txt).

Here's a quick example of how to build it on Linux:

```bash
# Clone the repo and all submodules
git clone --recurse-submodules https://github.com/martinrotter/rssguard

# Create a build directory
mkdir build-dir

# Configure the project to build using Qt 6, and disable built-in web browser support
cmake -B build-dir -S . -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_WITH_QT6=ON -DNO_LITE=OFF

# Compile it (in parallel mode)
cmake --build build-dir -j$(nproc)

# (Optional) Run the build to test it
./build-dir/src/rssguard/rssguard

# (Optional) Install RSS Guard system-wide
sudo make -C build-dir install
```
