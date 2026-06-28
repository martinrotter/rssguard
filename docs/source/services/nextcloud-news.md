Nextcloud News
==============
```{image} images/nextcloud-news.png
:alt: Nextcloud logo
:width: 64px
:align: right
```

The Nextcloud News plugin connects RSS Guard to the [News app](https://github.com/nextcloud/news) installed on a Nextcloud server. Subscriptions, folders and article states remain stored on the server and can be used from the Nextcloud web interface and other News clients.

Use this plugin when your feeds are already managed by Nextcloud News. For feeds stored only on the current computer, use the [standard feeds plugin](standard) instead.

## Before You Begin
You need:

* access to a Nextcloud server
* the Nextcloud News app installed and enabled
* your Nextcloud username
* a password accepted by client applications

RSS Guard currently requires Nextcloud News version 27.2.0 or newer. The **Test setup** action checks the installed version.

Use an `https://` server address whenever possible because the News API authenticates every request with your username and password.

## Passwords and Two-Factor Authentication
When two-factor authentication is enabled for your Nextcloud account, create a dedicated app password for RSS Guard:

1. Sign in to Nextcloud in a web browser.
2. Open your personal settings.
3. Open **Security**.
4. Find the section for devices or app passwords.
5. Enter a recognizable name such as `RSS Guard`.
6. Generate the password and enter it in RSS Guard.

The generated password is shown only once. It can later be revoked from Nextcloud without changing your normal account password.

An app password is also a good choice without two-factor authentication because it limits the effect of revoking RSS Guard's access.

## Add the Account
1. Open `Accounts -> Add account`.
2. Select `Nextcloud News`.
3. Enter the main address of your Nextcloud server, for example `https://cloud.example.com`.
4. Do not append a News API path to the address.
5. Enter your Nextcloud username.
6. Enter your app password or account password.
7. Select **Test setup**.
8. Review the download options and save the account after the test succeeds.

RSS Guard adds the News API path automatically. If Nextcloud is installed in a subdirectory, include that subdirectory in the server address, for example `https://example.com/nextcloud`.

```{warning}
Do not include your password or app password in screenshots, logs or bug reports.
```

## What Is Synchronized
The plugin synchronizes:

* feed subscriptions
* Nextcloud News folders
* read and unread state
* Nextcloud News starred state as RSS Guard importance
* article contents and enclosures supplied by News

Read and starred changes made in RSS Guard are cached locally and sent back to Nextcloud during synchronization.

RSS Guard labels are local when used with this plugin. They are not synchronized with Nextcloud News.

## Managing Feeds and Folders
You can add a feed from RSS Guard and place it at the account root or in an existing Nextcloud News folder. Deleting the feed in RSS Guard unsubscribes from it on the server.

Manage folders in the Nextcloud News web interface. After creating, renaming, moving or deleting folders and subscriptions there, refresh the account in RSS Guard to download the updated structure.

```{important}
Removing a Nextcloud News feed in RSS Guard changes the remote account too. It is not merely removed from the local database.
```

## Download Options
### Download Unread Articles Only
Enable **Download unread articles only** to request only unread articles from each feed.

This keeps the local database smaller, but read articles that are not already stored locally will not be downloaded. Leave it disabled when you want a more complete local history.

### Article Limit
**Only download newest X articles per feed** limits the number of articles requested from each subscription.

The default is 100. Set it to `0` for an unlimited request, but use that carefully with old or large accounts because synchronization and local database growth can increase substantially.

The Nextcloud News plugin does not use RSS Guard's intelligent synchronization algorithm.

## Server-Side Feed Updates
Nextcloud normally updates feeds independently through its configured background jobs. RSS Guard then downloads the results already stored by News. This is the recommended arrangement.

Enable **Force execution of server-side feeds update** only when you specifically need RSS Guard to ask News to refresh every feed before downloading its articles.

This option:

* makes synchronization noticeably slower
* can cause timeouts on large accounts
* invokes the update endpoint once for every feed
* generally requires a Nextcloud administrator account

If feeds remain stale with this option disabled, ask the server administrator to check the Nextcloud background-job and News updater configuration. The [Nextcloud News troubleshooting guide](https://nextcloud.github.io/news/troubleshooting/) contains server-side guidance.

## Proxy Settings
The account dialog includes RSS Guard's common proxy settings. They apply to the setup test, synchronization, feed management and icon downloads.

## Supported Features
The Nextcloud News plugin supports:

* two-way read and unread synchronization
* two-way starred-state synchronization
* synchronized subscriptions and folders
* adding feeds to existing folders
* remote feed removal
* unread-only downloading
* per-feed article limits
* optional server-side feed refresh
* account-specific proxy settings

## Limitations
* Nextcloud News 27.2.0 or newer is required.
* RSS Guard labels are not synchronized.
* Intelligent synchronization is not available.
* Folders must be managed through Nextcloud News.
* Unlimited history can be slow and consume substantial storage.
* Forced server-side updates are slow, can time out, and normally require administrator access.

For details about the server interface, see the official [Nextcloud News API documentation](https://nextcloud.github.io/news/api/api-v1-3/).
