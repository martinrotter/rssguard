Built-in Web Browser & AdBlock
==============================
Web browser capabilities differ depending on which RSS Guard [flavor](../variants) you install.

## AdBlock
Both variants of RSS Guard offer ad-blocking functionality via [Adblocker](https://github.com/cliqz-oss/adblocker). Adblocker offers similar performance to [uBlock Origin](https://github.com/gorhill/uBlock).

If you want to use AdBlock, you need to have Node.js installed.

You can find elaborate lists of AdBlock rules [here](https://easylist.to). You can simply copy the direct hyperlinks to those lists and paste them into the `Filter lists` text-box as shown below. Remember to always separate individual links with newlines. The same applies to `Custom filters`, where you can insert individual filters, for example [filter](https://adblockplus.org/filter-cheatsheet) "idnes" to block all URLs with "idnes" in them.

<img alt="alt-img" src="images/adblock.png" width="350px">

The way ad-blocking internally works is that RSS Guard starts local HTTP server which provides ad-blocking API, which is subsequently called by RSS Guard. There is some caching done in between, which speeds up some ad-blocking decisions.

## Node.js
RSS Guard has the [Node.js](https://nodejs.org) integration. For more information see `Node.js` section of RSS Guard `Settings` dialog.

Node.js is used for some advanced functionality like AdBlock.