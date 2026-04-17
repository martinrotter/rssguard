Displaying Articles
===================
RSS Guard uses an embedded `QtWebEngine`-based viewer to display articles with modern HTML and CSS rendering.

For most users, this means articles are shown with much better layout fidelity than a minimal text-only or lightweight HTML widget. Complex pages, modern styling, and richer embedded content generally behave much closer to what you would expect in a regular web browser.

## What You Can Expect
The built-in article viewer supports:
* Modern article rendering based on `QtWebEngine`
* Text search inside the currently displayed article
* Zooming in and out
* Opening links in a new tab or in an external browser
* Printing
* Printing directly to `PDF`
* Saving the current page as plain text, `HTML`, or complete webpage archive formats where applicable

## External Images And Other Remote Content
RSS Guard can load remote images inside displayed articles. This can improve visual fidelity, but it may also contact third-party servers when an article is opened.

If you prefer stricter privacy, disable loading of external images in the article viewer or leave it disabled by default and enable it only when needed.

## Advanced Actions
The article viewer context menu contains some advanced actions for power users, such as:
* Reloading while bypassing cache
* Viewing page source
* Tweaking selected `QtWebEngine` web attributes

These options are mainly useful for troubleshooting, privacy tuning, or handling unusual pages.

## Notes
* The exact rendering can still depend on the RSS/Atom content provided by the feed itself.
* Some content may open in a dedicated browser tab or in your external browser instead of navigating directly inside the article pane.
* Features and behavior can differ slightly between platforms and package variants.
