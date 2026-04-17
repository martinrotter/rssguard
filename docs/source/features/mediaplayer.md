Media Player
============
RSS Guard offers some of the capabilities everyone would expect from a typical podcast client. It offers a built-in media player on some platforms:
* Some RSS Guard builds offer a `libmpv`-based[^1] media player that supports almost all features offered by `libmpv`, including keyboard and mouse navigation, an on-screen controller, and loading custom configuration files. Also, `yt-dlp` is bundled on some[^2] platforms, which allows convenient YouTube video playback.
* Some RSS Guard builds include a more basic media player based on the `QtMultimedia` backend. This backend is usually based on `ffmpeg`, so it can play a fairly wide range of media formats.

[^1]: Official Windows `win10` packages use this backend. At this point, precompiled binaries for macOS do not have it enabled and rely on the `QtMultimedia` backend instead.
[^2]: `yt-dlp` is bundled with official Windows `win10` packages. On other operating systems, I recommend installing `yt-dlp` or `youtube-dl` system-wide and adding it to your `PATH` environment variable. The custom path to the tool can also be tweaked in `mpv.conf`.

## Usage
This feature is available for each hyperlink URL. You can either right-click any link in the embedded article or web browser, or use the `Articles -> Play in media player` menu item.

## Configuration
See the `Settings -> Media player` configuration section in RSS Guard. The user-data [placeholder](userdata) is supported.

## `libmpv` Backend
Note that this backend supports the full `mpv` configuration mechanism. You can set the path to a custom configuration folder in settings, as described above. If you select an empty folder, RSS Guard automatically copies sample configuration files into it.
