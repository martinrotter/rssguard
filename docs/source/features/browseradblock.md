Built-in Web Browser & AdBlock
==============================
RSS Guard is distributed in two variants:
* **Standard package with WebEngine-based bundled article viewer**: This variant displays messages/articles with their full formatting and layout in embedded Chromium-based web browser. This variant of RSS Guard should be okay for everyone. Also, installation packages are relatively big.

* **Lite package with simple text-based article viewer**: This variant displays article in a much simpler and much more lightweight web viewer component. All packages of this variant have `nowebengine` keyword in their names. This flavor of RSS Guard does NOT have a JavaScript support and is meant for people who value their privacy.

## AdBlock
Both variants of RSS Guard offer ad-blocking functionality via [Adblocker](https://github.com/cliqz-oss/adblocker). Adblocker offers similar performance to [uBlock Origin](https://github.com/gorhill/uBlock).

If you want to use AdBlock, you need to have Node.js installed.

You can find elaborate lists of AdBlock rules [here](https://easylist.to). You can simply copy the direct hyperlinks to those lists and paste them into the `Filter lists` text-box as shown below. Remember to always separate individual links with newlines. The same applies to `Custom filters`, where you can insert individual filters, for example [filter](https://adblockplus.org/filter-cheatsheet) "idnes" to block all URLs with "idnes" in them.

<img alt="alt-img" src="images/adblock.png" width="350px">

The way ad-blocking internally works is that RSS Guard starts local HTTP server which provides ad-blocking API, which is subsequently called by RSS Guard. There is some caching done in between, which speeds up some ad-blocking decisions.

## Node.js
RSS Guard has the [Node.js](https://nodejs.org) integration. For more information see `Node.js` section of RSS Guard `Settings` dialog.

Node.js is used for some advanced functionality like AdBlock.