Tiny Tiny RSS
=============
```{image} images/tiny-tiny-rss.png
:alt: Tiny Tiny RSS logo
:width: 64px
:align: right
```

The Tiny Tiny RSS plugin connects RSS Guard to a self-hosted [Tiny Tiny RSS](https://tt-rss.org/) instance. Subscriptions, folders, article states and labels remain stored on the server and can be used from the TT-RSS web interface and other clients.

RSS Guard requires TT-RSS API level 9 or newer. The **Test setup** action checks the available API level.

## Enable API Access
Tiny Tiny RSS can enable or disable API access separately for each user:

1. Sign in to TT-RSS in a web browser.
2. Open **Preferences**.
3. Find **Enable API access** in the user preferences.
4. Enable it and save the preference.

The exact location can vary between TT-RSS versions and themes. Ask the server administrator when the setting is unavailable.

Use an `https://` address whenever possible so that credentials and synchronized data are encrypted in transit.

## Add the Account
1. Open `Accounts -> Add account`.
2. Select `Tiny Tiny RSS`.
3. Enter the address of the TT-RSS installation, for example `https://reader.example.com/tt-rss`.
4. Do not append `/api` or `/api/` to the address.
5. Enter your TT-RSS username and password.
6. Select **Test setup**.
7. Review the synchronization options and save the account after the test succeeds.

RSS Guard adds the API path automatically. The test reports whether API access is disabled, the credentials are incorrect, or the server provides an unsupported API level.

```{warning}
Do not include your TT-RSS password in screenshots, logs or bug reports.
```

## HTTP Authentication
The normal username and password fields authenticate your TT-RSS account.

Enable **Requires HTTP authentication** only when the web server or reverse proxy protecting the entire TT-RSS site asks for a second username and password before TT-RSS itself is reached. Enter those outer credentials in the HTTP authentication section.

Most installations do not need this option. It is separate from credentials used by TT-RSS to download an individually protected feed.

## What Is Synchronized
The plugin synchronizes:

* feed subscriptions
* TT-RSS folders
* read and unread state
* TT-RSS starred state as RSS Guard importance
* TT-RSS labels and their article assignments
* Published state
* article contents and enclosures supplied by TT-RSS

Changes to read state, importance, labels and Published state are cached locally and uploaded during synchronization.

Create, rename or remove label definitions in TT-RSS. Synchronize the account afterward to refresh them in RSS Guard.

## Managing Feeds and Folders
You can add a feed from RSS Guard and place it in an existing TT-RSS folder. When the feed itself requires HTTP Basic authentication, enter those credentials on the **Auth** tab of the add-feed dialog; RSS Guard passes them to TT-RSS while creating the subscription.

Deleting a feed in RSS Guard unsubscribes from it on the server.

Manage folders and reorganize existing subscriptions in the TT-RSS web interface, then refresh the account in RSS Guard. RSS Guard does not create TT-RSS folders.

```{important}
Removing a TT-RSS feed in RSS Guard changes the remote account too. It is not merely removed from the local database.
```

## Synchronization Methods
### Standard Synchronization
Standard synchronization uses the regular TT-RSS API and downloads a requested set of recent articles from each feed. It works without an additional TT-RSS plugin.

Use **Only download newest X articles per feed** to control the amount of history. The default is 100. A value of `0` removes RSS Guard's overall limit, although requests are still transferred in batches supported by the TT-RSS API.

### Intelligent Synchronization
**Intelligent synchronization algorithm** first compares remote identifiers and article states with RSS Guard's local database, then downloads only missing or changed articles. This normally reduces repeated downloads and improves later synchronization speed.

This mode requires the `api_newsplus` plugin on the TT-RSS server because RSS Guard uses its `getCompactHeadlines` API method. The server administrator must install and enable that plugin before intelligent synchronization is selected. Without it, feed updates fail with a message that the method is not installed.

The article limit is not used in intelligent mode. The initial synchronization can still take time because RSS Guard must compare the remote and local article sets.

## Download Unread Articles Only
Enable **Download unread articles only** to focus synchronization on the unread queue.

Articles that are already read on the server and are not stored locally will generally be omitted. Starred and changed articles can still be downloaded when needed for state synchronization.

Leave this option disabled when you want a more complete local article history.

## Force Server-Side Feed Updates
TT-RSS normally refreshes feeds through its own background updater. RSS Guard then downloads the articles already stored on the server. This is the recommended setup.

With standard synchronization, **Force execution of server-side feeds update** asks TT-RSS to refresh each feed immediately before returning its articles. TT-RSS performs this work synchronously, so account updates can become much slower and may time out.

This option does not drive the intelligent synchronization path. If articles remain stale, check the TT-RSS background updater before enabling forced refresh.

## Published Articles and Notes
TT-RSS exposes Published state in two related ways:

* **Published articles** appears as a synchronized RSS Guard label. Assigning or removing it changes the article's Published state on the server.
* **User-published articles** is a special feed containing notes created through TT-RSS's `shareToPublished` API.

To create a note, open the context menu of the **User-published articles** feed and select **Share to published**. Enter a title, URL and optional plain-text content. The resulting item is stored by TT-RSS and appears after synchronization.

## Proxy Settings
The account dialog includes RSS Guard's common proxy settings. They apply to login, synchronization, feed management, notes and icon downloads.

## Supported Features
The Tiny Tiny RSS plugin supports:

* two-way read and unread synchronization
* two-way starred-state synchronization
* synchronized TT-RSS labels
* Published article synchronization
* creating Published notes
* standard and intelligent synchronization
* adding feeds to existing folders
* authenticated feed subscription
* remote feed removal
* unread-only downloading
* article limits for standard synchronization
* optional server-side feed refresh
* optional site-wide HTTP authentication
* account-specific proxy settings

## Limitations
* TT-RSS API level 9 or newer is required.
* API access must be enabled for the TT-RSS user.
* Intelligent synchronization requires the separate `api_newsplus` server plugin.
* RSS Guard does not create or reorganize TT-RSS folders.
* The standard-mode article limit and forced refresh do not apply to intelligent synchronization.
* Forced server-side updates can be slow and may time out.
* Unread-only mode intentionally creates an incomplete local article history.

For server API details, see the official [Tiny Tiny RSS API reference](https://tt-rss.org/docs/API-Reference.html).
