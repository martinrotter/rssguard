Displaying Articles
===================
RSS Guard has a built-in GUI component for displaying articles with good formatting. Below you can find some specific information about the supported article display components.

## `litehtml`
The default component for displaying articles is based on `litehtml`. [`litehtml`](http://www.litehtml.com) is a lightweight `HTML` viewer that supports almost complete `CSS` support and is able to display articles with good formatting. Its main focus is speed and a very low memory footprint when compared to much more robust alternatives such as `QtWebEngine`. The resulting binaries are also much slimmer. These advantages are unfortunately accompanied by some limitations, listed below.

Here are some key points of this component:
* No support for direct `PDF` printing.
* Text and `URL` drag support works only with the mouse **MIDDLE** button (wheel) pressed.
* Text selection itself might have some quirks.
* There is no right-to-left support.
* No support for `GIF` animations.
* No direct media playback, although you can of course use the bundled media player feature.

## `QtWebEngine`
```{warning}
`QtWebEngine` was removed for RSS Guard 5.
```
