Displaying Articles
===================
RSS Guard has built-in GUI component to display articles with good formatting. Below you can see some specific information about supported article display components.

## `litehtml`
The default component for displaying articles is `litehtml`-based. [`litehtml`](http://www.litehtml.com) is lightweight `HTML` viewer which supports almost complete `CSS` and is able to display articles with good formatting. Its main focus is speed and very low memory footprint when compared to much more robust alternatives like `QtWebEngine`. Also, the resulting binaries are much much slimmer. These advantages are sadly complemented by some not-so-good things, see below.

Here are some key points of this component:
* No support for direct `PDF` printing.
* Text and `URL` drag support works only with mouse **MIDDLE** button (wheel) pressed.
* Text selection itself might have some quirks.
* There is not right-to-left support.
* No support for `GIF` animations.
* No direct media playback (you can of course use bundled media player feature).

## `QtWebEngine`
```{warning}
`QtWebEngine` was removed for RSS Guard 5.
```