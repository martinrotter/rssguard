Feedly
======
```{image} images/feedly.png
:alt: Feedly logo
:width: 64px
:align: right
```

The Feedly plugin connects RSS Guard to an existing [Feedly](https://feedly.com) account. Your subscriptions, collections, article states and Feedly tags remain stored by Feedly and can be used from other Feedly clients too.

Use this plugin when Feedly is your main synchronization service. If you only want to store ordinary feeds locally in RSS Guard, use the [standard feeds plugin](standard) instead.

## Before You Begin
Authentication depends on how RSS Guard was packaged:

* **Official RSS Guard builds** include Feedly OAuth support. You sign in through your web browser and do not need a developer access token.
* **Unofficial or self-built packages** may not contain Feedly OAuth credentials. In that case the setup dialog asks for a Feedly developer access token.

The help text in the account dialog tells you which method your current build supports.

## Setup with an Official Build
1. Open `Accounts -> Add account`.
2. Select `Feedly`.
3. Leave **Developer access token** empty.
4. Select **Login**.
5. Sign in to Feedly in the browser and grant RSS Guard access.
6. Return to RSS Guard after the connection test succeeds.
7. Review the synchronization options and save the account.

RSS Guard obtains the account identity from Feedly and fills in the username. You may see another authorization prompt when the newly saved account starts for the first time.

## Setup with a Developer Token
When the dialog says that official Feedly support is unavailable:

1. Select **Get token** to open Feedly's developer-token page.
2. Sign in to Feedly and generate a token.
3. Enter your Feedly username or email address.
4. Paste the token into **Developer access token**.
5. Select **Login** to test it.
6. Save the account after the test succeeds.

Feedly developer tokens are usually valid for about one month and are limited to approximately 250 API requests per day. When a token expires, generate a replacement and edit the account in RSS Guard.

```{warning}
Treat a developer token like a password. Do not include it in screenshots, logs or bug reports.
```

## What Is Synchronized
RSS Guard downloads the Feedly account structure and articles. It synchronizes:

* subscriptions
* Feedly collections as folders
* read and unread state
* Feedly **Saved** state as RSS Guard importance
* existing Feedly tags as RSS Guard labels
* assigning and removing those tags on articles

Changes to read state, importance and tag assignments are queued locally and sent back to Feedly during synchronization.

Feed subscriptions, collections and tags should be created, renamed or removed in Feedly. Refresh the account in RSS Guard afterward to download the updated structure.

```{note}
RSS Guard displays each feed in one folder. If the same Feedly subscription belongs to several collections, it cannot be duplicated at several places in the RSS Guard feed tree.
```

## Synchronization Options
### Download Unread Articles Only
Enable **Download unread articles only** when you only need the current unread queue.

This reduces downloads, but previously read Feedly articles that are not already in the RSS Guard database will not be added. Leave it disabled if you want a more complete local article history.

### Intelligent Synchronization
**Intelligent synchronization algorithm** is recommended. RSS Guard first compares remote article identifiers and states with its local database, then downloads only missing or changed articles.

This usually transfers much less data and makes later updates faster. The first synchronization can still take time for accounts with many subscriptions or a large history.

### Article Limit
**Only download newest X articles per feed** limits how much history RSS Guard requests for each subscription.

The default is 100 articles per feed and is a sensible starting point. Feedly retains a large article history, so requesting too many articles can create a lengthy first synchronization and a much larger RSS Guard database.

## Proxy Settings
The account dialog includes the common RSS Guard proxy settings. Use them when Feedly must be reached through a particular HTTP or SOCKS proxy.

The proxy is used for RSS Guard's Feedly connection test and API requests. The external browser used for OAuth follows that browser's own network configuration.

## Everyday Use
Use the normal account update action to:

* refresh subscriptions and collections
* download article changes
* upload cached read, unread, saved and tag changes

If authorization expires, RSS Guard displays a login notification. Follow its **Login** action to authorize the account again. Developer-token users need to replace the token manually when it expires.

## Supported Features
The Feedly plugin supports:

* two-way article-state synchronization
* intelligent synchronization
* synchronized Feedly tags
* OAuth in official RSS Guard builds
* developer-token authentication in other builds
* per-account article limits
* unread-only downloading
* account-specific proxy settings

## Limitations
* RSS Guard does not add or reorganize Feedly subscriptions and collections. Manage them in Feedly.
* Feedly tags can be assigned and removed in RSS Guard, but their definitions are managed by Feedly.
* Downloading only unread articles intentionally produces an incomplete local history.
* Developer-token authentication has stricter expiration and daily request limits than official OAuth support.
