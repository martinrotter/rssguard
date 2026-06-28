Google Reader API
=================
```{image} images/google-reader-api.png
:alt: Google Reader API plugin icon
:width: 64px
:align: right
```

The Google Reader API plugin connects RSS Guard to online readers and self-hosted servers that provide a Google Reader-compatible API.

RSS Guard has dedicated presets for:

* ![Bazqux logo](images/bazqux.png){width="32px"} **Bazqux**
* ![FreshRSS logo](images/freshrss.png){width="32px"} **FreshRSS**
* ![Inoreader logo](images/inoreader.png){width="32px"} **Inoreader**
* ![Miniflux logo](images/miniflux.png){width="32px"} **Miniflux**
* ![Reedah logo](images/reedah.png){width="32px"} **Reedah**
* ![The Old Reader logo](images/theoldreader.png){width="32px"} **The Old Reader**

An **Other** option is available for compatible servers that are not listed. Compatibility depends on how completely the server implements the API.

## Before You Begin
Create your account and subscriptions on the chosen service first. For a self-hosted service, make sure its Google Reader-compatible API is enabled and that you know the server address and credentials accepted by that API.

The address entered in RSS Guard is the main server address, without an API-specific path. For example, enter:

```text
https://reader.example.com
```

Do not append paths such as FreshRSS's `api/greader.php`. RSS Guard adds the required service-specific path itself.

## Setup with a Username and Password
This method applies to Bazqux, FreshRSS, Miniflux, Reedah, The Old Reader and compatible servers selected through **Other**.

1. Open `Accounts -> Add account`.
2. Select `Google Reader API`.
3. Choose your service.
4. Enter the server address. RSS Guard fills it automatically for several hosted services.
5. Enter the username and password used by the service's Google Reader-compatible API.
6. Select **Test setup**.
7. Review the synchronization options and save the account after the test succeeds.

Some self-hosted readers use a separate API password or require API access to be enabled in their web settings. Follow the documentation for your server when its normal web password is not accepted.

```{warning}
Treat an API password like your normal account password. Do not include it in screenshots, logs or bug reports.
```

## Setup with Inoreader
Inoreader uses OAuth instead of the username-and-password form.

### Preconfigured OAuth
Official RSS Guard builds can include preconfigured Inoreader credentials:

1. Choose **Inoreader** as the service.
2. Leave **App ID**, **App key** and **Redirect URL** at their defaults, even when the first two fields are empty.
3. Select **Test setup**.
4. Sign in to Inoreader in the browser and grant access.
5. Return to RSS Guard after the test succeeds and save the account.

The preconfigured application has a quota shared by its users. If authorization is unavailable or the shared quota is exhausted, use your own Inoreader application.

### Your Own Inoreader Application
1. Select **Get my own App ID** in the account dialog.
2. Register an application with Inoreader.
3. Copy its App ID and App key into RSS Guard.
4. Configure the application with exactly the redirect URL shown by RSS Guard.
5. Select **Test setup** and complete authorization in the browser.
6. Save the account after the test succeeds.

Keep the App key private.

## What Is Synchronized
The plugin synchronizes:

* feed subscriptions
* service folders
* read and unread state
* starred articles as RSS Guard importance
* service labels and their article assignments, where supported

Changes to article state and label assignments are cached locally and uploaded to the service during synchronization.

The Old Reader does not provide synchronized article labels through this integration. Its folders and article states are still synchronized.

## Managing Feeds
You can add a feed from RSS Guard and choose an existing service folder for it. RSS Guard creates the subscription on the server and then refreshes the account tree.

You can also:

* rename a subscription
* move it between existing folders
* unsubscribe by deleting it in RSS Guard

These operations affect the remote account, not just the local RSS Guard database. Folder creation and other advanced subscription organization are best performed in the service's web interface, followed by an account refresh in RSS Guard.

The account menu also provides **Import feeds** and **Export feeds** actions. These send or receive an OPML file through the service API, so support can vary between compatible servers.

## Synchronization Options
### Download Unread Articles Only
Enable **Download unread articles only** to keep the local database focused on your unread queue.

Read articles that are not already stored locally will generally not be downloaded. Leave this disabled when you want a more complete local history.

### Intelligent Synchronization
**Intelligent synchronization algorithm** compares remote article identifiers and states with the local database, then downloads only missing or changed articles.

It is recommended for most accounts because it reduces network traffic and makes later updates faster. The first synchronization can still take time for a large account.

Miniflux requires intelligent synchronization because its compatible API does not provide the older full-stream operation used by RSS Guard. The option is therefore always enabled for Miniflux.

### Fetch Articles Newer Than
Use **Fetch articles newer than** to set the oldest date RSS Guard should consider during intelligent synchronization. A recent date reduces the work needed for the initial synchronization.

### Article Limit
When intelligent synchronization is disabled, **Only download newest X articles per feed** limits the amount of history requested from each subscription.

The default is 100 articles per feed. Increase it carefully for large accounts. This limit is not used by the intelligent synchronization algorithm.

## Proxy Settings
The account dialog includes RSS Guard's common proxy settings. They apply to connection tests, synchronization, feed-management requests and OPML transfers.

The external browser used for Inoreader authorization follows the browser's own network configuration.

## Supported Features
The Google Reader API plugin supports:

* two-way article-state synchronization
* intelligent synchronization
* synchronized labels when provided by the service
* adding, editing, moving and removing subscriptions
* remote OPML import and export
* Inoreader OAuth
* username-and-password authentication for other services
* unread-only downloading
* synchronization date and article limits
* account-specific proxy settings

## Service Notes and Limitations
* Google Reader-compatible APIs are not completely uniform. A server selected through **Other** may omit operations that RSS Guard uses.
* Miniflux always uses intelligent synchronization.
* Inoreader is the only preset that uses OAuth.
* The Old Reader does not synchronize article labels.
* OPML import and export depend on the server implementing the corresponding API operations.
* Unread-only mode intentionally creates an incomplete local article history.
