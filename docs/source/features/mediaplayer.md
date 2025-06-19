Media Player
============
RSS Guard offers some abilities everyone would expect from typical podcast client. It offers built-in media player on some platforms:
* some RSS Guard builds offer `libmpv`-based[^1] media player which supports almost all features offered by `libmpv`, including keyboard/mouse navigation, on-screen controller and custom configuration files loading. Also, `yt-dlp` is deployed on some[^2] platforms which allows comfortable Youtube videos playback. 
* some RSS Guard builds include more basic media player based on `QtMultimedia` backend. This backend is usually based on `ffmpeg` so it can play fairly wide range of media formats.

[^1]: At this point, precompiled binaries for Mac OS X do not have this backend enabled and rely on `QtMultimedia` backend instead.
[^2]: `yt-dlp` is contained in binaries for Windows. For other operating systems, I recommend to install `yt-dlp` or `youtube-dl` system-wide and add it to your `PATH` environment variable. Also, custom path to the tool can be tweaked in `mpv.conf`.

## Usage
This feature is available for each hyperlink URL. You can either right-click any link in embedded article/web browser or you can use `Articles -> Play in media player` menu item.

## Configuration
See `Settings -> Media player` configuration section in RSS Guard. User data [placeholder](userdata) is supported.

## `libmpv` Backend
Note that this backend does support full `mpv` configuration mechanism. You can set path to custom configuration folder in settings (see above). If you select empty folder, then RSS Guard automatically copies sample configuration files into it.