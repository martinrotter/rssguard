Standard RSS, Atom, JSON and Other Feeds
========================================
```{image} images/standard.svg
:alt: Standard feeds icon
:width: 64px
:align: right
```

The **Local RSS/RDF/ATOM/JSON** plugin stores feeds directly in RSS Guard. Use it when you want to subscribe to ordinary feed URLs without synchronizing through an online feed-reader service.

It is also the most flexible plugin. Besides normal web feeds, it can read local files, run scripts, import and export OPML, discover feeds from websites and apply network settings to individual feeds.

## Add the Account
1. Open `Accounts -> Add account`.
2. Select `Local RSS/RDF/ATOM/JSON`.
3. Choose a title and icon for the account.
4. Optionally adjust **Feed fetch spacing**. A short delay between feed downloads can reduce load on servers and avoid starting every request at once.
5. Save the account.

When the account is empty, RSS Guard can offer to import data from RSS Guard 4, QuiteRSS or an OPML file. You can also load the bundled starter feeds or begin with an empty account.

## Supported Sources
The plugin recognizes:

* RSS 0.x and RSS 2.x
* Atom 1.0
* RDF/RSS 1.0
* JSON Feed 1.0 and 1.1
* XML sitemaps, including sitemap indexes and compressed sitemaps
* iCalendar
* Gemlogs

Normal feeds can be downloaded over HTTP or HTTPS. RSS Guard can also fetch feeds through the [Gemini](https://geminiprotocol.net) protocol.

Podcasts work like other feeds. Their media attachments can also be opened in RSS Guard's [media player](../features/mediaplayer) when that feature is available.

## Add and Discover Feeds
For the easiest setup:

1. Select the account or one of its folders.
2. Choose `Feeds -> Add item -> Add a new feed`.
3. Enter a feed URL or the address of a website.
4. Select the discovered feeds you want to add.

You do not always need the exact feed URL. RSS Guard downloads the supplied website, looks for advertised feed links and checks the resulting feed candidates.

<img alt="Feed discovery dialog" src="../features/images/discover-feeds.png">

The dialog accepts multiple addresses, one per line. Deep discovery can inspect links found in the supplied pages too, but it may download many pages and take noticeably longer.

Use **Switch to advanced mode** when you need to add one feed with custom source or network settings.

<img alt="Advanced standard feed details" src="../features/images/feed-details.png">

For a normal feed, enter its address in **Source** and select **Fetch metadata**. RSS Guard attempts to detect the format, title, description, encoding and icon.

## Folders and Local Storage
Feeds can be organized into folders and rearranged freely. Articles, read states and account structure are stored in RSS Guard's own database; they are not synchronized to another feed reader.

RSS Guard labels still work, but they remain local for this account.

## OPML Import and Export
The standard plugin can import and export OPML files. Open the account menu and choose its import or export action.

<img alt="OPML import menu" src="../features/images/opml-import-menu.png" width="600px">

During import you can:

* choose the destination folder
* select which feeds should be imported
* fetch feed titles and icons
* skip online metadata for a faster import
* apply an optional post-processing command

Fetching online metadata can take some time for a large subscription list.

<img alt="OPML import dialog" src="../features/images/opm-import-windows.png" width="600px">

Export can include selected feeds, folder structure and feed icons.

## Feed Sources and Processing
Most feeds use a normal URL, but the advanced feed dialog supports three source types:

* **URL** downloads a remote feed.
* **Local file** reads feed data generated or stored on your computer.
* **Script** runs a command and reads feed data from its standard output.

An optional post-processing command can transform downloaded or generated data before RSS Guard parses it.

See [Scripts, local files and post-processing](standard-sources) for setup details and examples.

## Full Articles and Comments
The **Experimental** feed settings include:

* **Fetch full articles** to replace abbreviated feed contents with extracted website contents
* **Fetch in plain text** to discard extracted HTML formatting
* **Fetch comments for articles** when the feed and target pages expose supported comment links
* **Article date preference** to prefer the published or updated timestamp
* **Report feed as broken when it contains no articles**

Full-article and comment fetching performs extra downloads, so feed updates can take longer and the database can grow more quickly.

## Per-Feed Network Settings
The **Network** tab can override application or account behavior for one feed:

* connection timeout
* HTTP/2 preference
* cookie use
* proxy and proxy credentials
* additional proxy domains used by article resources in the `web` viewer
* HTTP authentication
* custom HTTP headers

A timeout of `0` uses the application-wide feed timeout. Other options that offer an application or account default behave similarly.

Ignoring cookies is useful when a feed must never share cookies accepted in RSS Guard's built-in browser. Custom headers and authentication can help with private feeds, APIs or servers that require a particular request.

Additional proxy domains are mainly useful when article images or styles are hosted on a separate CDN. They affect proxy rules generated for the `web` viewer; see [Proxy use](../features/article-display.md#proxy-use).

## Useful Features
The standard plugin supports:

* automatic feed discovery
* manual feed and folder creation
* OPML import and export
* per-account feed download spacing
* per-feed update intervals
* conditional HTTP requests using server metadata
* custom icons, titles and descriptions
* podcasts and enclosures
* article filters, labels and queries
* local files, scripts and post-processing
* full-article extraction
* per-feed authentication, proxy and request settings

## Notes
* This plugin does not provide two-way synchronization with another feed-reader service.
* Moving the RSS Guard database or restoring a backup is how you transfer locally stored articles and read states.
* A website can change or remove its feed without notice. Script-based sources can also stop working when an upstream page or API changes.
* Only standard feeds are handled by the RSS Guard 4 and QuiteRSS import tools.

```{toctree}
:hidden:
:maxdepth: 1

standard-sources
```
