Displaying Articles And Web Pages
=================================
RSS Guard uses one embedded viewer for both article display and built-in web browsing. The exact viewer depends on the package or build you use.

Official package names include the viewer type:
* `web` - Qt WebEngine-based viewer
* `text` - QTextBrowser-based viewer

Both variants can display feed articles and open web pages, but they do not have the same feature set.

## `web` Viewer
The `web` variant uses Qt WebEngine. It is the most complete option and behaves much more like a regular web browser.

It is the best choice if you want:
* modern HTML and CSS rendering
* normal image loading in articles and web pages
* browser-like navigation with back, forward, and reload
* page actions provided by Qt WebEngine
* printing directly to `PDF`
* saving a complete page where supported
* more complete handling of complex websites

The tradeoff is size and complexity. Qt WebEngine packages are larger and include more runtime files.

## `text` Viewer
The `text` variant uses QTextBrowser. It is intentionally simpler and lighter.

It is a good choice if you want:
* a smaller and simpler build
* readable article display without a full embedded browser engine
* basic opening of web pages
* text search, zooming, printing, and saving as `HTML` or plain text

The `text` viewer has important limitations:
* it does not load article images as normal inline remote images
* it replaces image content with links or placeholders where possible
* it does not provide full browser navigation
* complex websites may not render correctly
* advanced Qt WebEngine actions are not available

## External Images And Privacy
The `web` viewer can load external images and other remote page resources. This gives better visual fidelity, but it can contact third-party servers when an article or page is opened.

If you prefer stricter privacy, disable external image loading or use the `text` variant.

## Cookies
In the `web` variant, cookies accepted or created in RSS Guard's built-in web browser are shared with RSS Guard's internal network stack. This means that if you sign in to a site in the built-in browser and the site stores a cookie, feeds from the same site that require that cookie may start working in RSS Guard too.

This is useful for feeds hidden behind simple cookie-based access, but it is not a replacement for real account synchronization or OAuth-based services. It also only applies to the `web` variant; the lighter `text` variant does not use Qt WebEngine.

## Proxy Use
Both viewer variants can use RSS Guard's account and feed proxy settings, but they do it differently.

The `text` variant uses RSS Guard's internal network code when it downloads a directly opened page or article images. These requests use the proxy configured for the current feed or its account. If there is no current feed context, the system/app default proxy behavior is used.

The `web` variant uses Qt WebEngine, which has its own network stack. RSS Guard therefore generates a local proxy auto-config (`PAC`) file for WebEngine. This allows web pages and article resources to follow per-account and per-feed proxy rules as closely as possible.

For WebEngine proxy rules, RSS Guard matches the feed host and related subdomains. For example, a feed from `feeds.bbc.co.uk` can also cover resources from `images.bbc.co.uk`. More specific rules are checked first, so a feed-specific proxy can still override a broader domain rule.

If a proxy needs authentication, RSS Guard can use credentials saved in the current feed/account proxy settings. If credentials are missing, the `web` variant may ask for them while the page is loading.

## Which Variant Should I Use?
If you are unsure, start with the `web` package. It gives the best rendering and the most complete built-in browsing experience.

Choose the `text` package if you prefer a lighter build, want fewer browser-like capabilities, or mostly read text-heavy feeds.

## Notes
* The exact rendering still depends on the feed contents and on the original website.
* Both viewers share common RSS Guard actions such as opening links externally, opening links in new tabs, copying selected text or links, printing, and playing supported links in the media player when that feature is available.
* Some options only appear when the selected viewer supports them.
