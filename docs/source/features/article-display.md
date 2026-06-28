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
* optional loading of remote article images without a full browser engine
* text search, zooming, printing, and saving as `HTML` or plain text
* readable display of directly opened image, `XML`, `JSON`, Markdown, `HTML`, and plain-text files

The `text` viewer has important limitations:
* remote article images are downloaded by RSS Guard and then inserted into the document; this is simpler than normal browser resource loading
* when external image loading is disabled, image content is replaced with links or placeholders where possible
* it does not provide full browser navigation
* complex websites may not render correctly
* advanced Qt WebEngine actions are not available

## External Images And Privacy
Both viewers can load external article images when external resources are enabled. This gives better visual fidelity, but it can contact third-party servers when an article or page is opened.

The `web` viewer lets Qt WebEngine load page resources in a browser-like way. The `text` viewer is more limited: it scans article/page `HTML` for images, downloads supported remote image URLs through RSS Guard's network code, caches them, and then displays the cached images in the document.

If you prefer stricter privacy, keep external image loading disabled. In that mode RSS Guard avoids downloading remote images automatically and the `text` viewer shows image links/placeholders where possible.

## Directly Opened Files
When you open a URL directly in RSS Guard, the active viewer decides how to display it.

The `web` viewer mostly follows normal browser behavior provided by Qt WebEngine.

The `text` viewer downloads the target through RSS Guard and then chooses a readable representation:
* image files are shown directly when external image loading is enabled; otherwise they are opened externally
* `XML` files are pretty-printed when possible
* `JSON` files are pretty-printed when possible
* Markdown files are rendered to readable `HTML`
* unknown text-like content is shown as escaped plain text

## Cookies
In the `web` variant, cookies accepted or created in RSS Guard's built-in web browser are shared with RSS Guard's internal network stack. This means that if you sign in to a site in the built-in browser and the site stores a cookie, feeds from the same site that require that cookie may start working in RSS Guard too.

This is useful for feeds hidden behind simple cookie-based access, but it is not a replacement for real account synchronization or OAuth-based services. It also only applies to the `web` variant; the lighter `text` variant does not use Qt WebEngine.

## Proxy Use
Both viewer variants can use RSS Guard's account and feed proxy settings, but they do it differently.

The `text` variant uses RSS Guard's internal network code when it downloads a directly opened page or article images. These requests use the proxy configured for the current feed or its account. If there is no current feed context, the system/app default proxy behavior is used.

The `web` variant uses Qt WebEngine, which has its own network stack. RSS Guard therefore generates a local proxy auto-config (`PAC`) file for WebEngine. This allows web pages and article resources to follow per-account and per-feed proxy rules as closely as possible.

For WebEngine proxy rules, RSS Guard matches the feed host and related subdomains. For example, a feed from `feeds.bbc.co.uk` can also cover resources from `images.bbc.co.uk`. More specific rules are checked first, so a feed-specific proxy can still override a broader domain rule.

The [standard feeds plugin](../services/standard.md#per-feed-network-settings) can also define extra proxy domains when article resources are hosted somewhere other than the feed's own domain.

```{warning}
Extra proxy domains only affect the `web` variant. They are used to generate Qt WebEngine `PAC` rules and do not change how the `text` variant loads resources.
```

```{warning}
Proxy `PAC` rules are generated when RSS Guard starts. Restart RSS Guard after changing account proxy, feed proxy or extra proxy domain settings if you need the built-in WebEngine browser to use the new rules immediately.
```

If a proxy needs authentication, RSS Guard can use credentials saved in the current feed/account proxy settings. If credentials are missing, the `web` variant may ask for them while the page is loading.

## Which Variant Should I Use?
If you are unsure, start with the `web` package. It gives the best rendering and the most complete built-in browsing experience.

Choose the `text` package if you prefer a lighter build, want fewer browser-like capabilities, or mostly read text-heavy feeds.

## Notes
* The exact rendering still depends on the feed contents and on the original website.
* Both viewers share common RSS Guard actions such as opening links externally, opening links in new tabs, copying selected text or links, printing, and playing supported links in the media player when that feature is available.
* The `text` viewer can show many simple pages, but it is not a full browser engine. JavaScript-heavy websites, complex layouts, advanced CSS and dynamic pages need the `web` variant or an external browser.
* Some options only appear when the selected viewer supports them.
