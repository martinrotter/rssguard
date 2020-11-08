# Accounts

RSS Guard supports the following integrations that enable synchronization across RSS clients. Using any of these **requires** a server running the software already installed and configured ahead of time.

- [Tiny Tiny RSS](https://tt-rss.org/)
- [Inoreader](https://www.inoreader.com/)
- [Nextcloud News](https://apps.nextcloud.com/apps/news)

## Tiny Tiny RSS

Before using this integration API access _must_ be enabled in Tiny Tiny RSS preferences. Any recent version of tt-rss will suffice.

1. Go to **Accounts → Add New Account**. Select **Tiny Tiny RSS**.
2. Enter the required information:
    - URL: `https://rss.domain.example/`. Do not enter any API information here, RSS will look for it automatically.
    - Download only unread messages
    - Force execution of server-side update when updating feeds from RSS Guard: This forces tt-rss to refresh all the feeds stored on the server. Depending on how many system resources are available to the server application this _will_ cause you issues.
    - Username
    - Password
    - If the server requires HTTP authentication enter that information. Only Basic HTTP Authentication is supported.
3. **Test** the connection before proceeding. If there are issues you will see an error message appear.
4. Click **OK**.

RSS Guard will start syncing your feeds.

## Inoreader

Before using this integration it is highly recommended you configure your own Inoreader client application according to [Inoreader documentation](https://www.inoreader.com/developers/register-app); when setting this up you _must_ configure the **Redirect URI** to `http://localhost` and the **Scope** to Read and write. Once this is done you can set it up in RSS Guard:

1. Go to **Accounts → Add New Account**. Select **Inoreader**.
2. Enter the required information:
    - Username: As entered to log in to Inoreader.
    - Application ID: As provided by Inoreader.
    - Application Key: As provided by Inoreader.
    - Redirect URL: `http://localhost`, as entered in the Inoreader OAuth 2.0 configuration page.
3. Click **Login**. This should open a browser window with an Inoreader authorization page.
4. Click **Authorize** and close the browser window.
5. In RSS Guard click **OK** to confirm the account addition.

RSS Guard will start syncing your feeds.

## Nextcloud News

[Nextcloud News](https://github.com/nextcloud/news) is an installable app that lets you view RSS within Nextcloud. RSS Guard can use it store the list of feeds and manipulate their state.

To set it up:

1. Go to **Accounts → Add New Account**. Select **Nextcloud News**.
2. Enter the required information:
    - URL: `https://nextcloud.server.example`. Do not enter any API or WebDAV information. RSS Guard will look for it automatically.
    - Download only unread messages
    - Force execution of server-side update when updating feeds from RSS Guard forces Nextcloud to refresh all the feeds stored. Depending on how many system resources are available to Nextcloud this _will_ cause you issues.
    - Limit number of downloaded messages per feed. Limiting this is useful if you're on a mobile or rate-limited connection.
    - Username: As it would be to log in to your Nextcloud instance.
    - Password

The minimum supported version of Nextcloud News is 6.0.5; any version of Nextcloud released in the last year should be compatible.

## Standard online feeds (RSS/RDF/ATOM)

This account type lets RSS Guard function as a stand-alone RSS client. It saves feeds and their state to the database and does not sync it with any external servers or applications. This _can_ be useful if you wish to sync the entire RSS Guard database if you do not plan on using any of the available integrations. Be aware the database can grow to huge sizes and you will see increased network traffic as your syncing application will have to keep up with the manipulation of feeds within RSS Guard itself.

1. Go to **Accounts → Add New Account**. Select Standard online feeds (RSS/RDF/ATOM)
2. You will be given the option to load a default set of feeds. If you select **Yes** the following folders and feeds will be created:
    - Fun & Life: [Interesting Thing of the Day](http://feeds.feedburner.com/InterestingThingOfTheDay), [Mashable](http://feeds.feedburner.com/Mashable?format=xml), [SimplyRecipes.com](http://feeds.feedburner.com/elise/simplyrecipes)
    - News: [BBC International News](http://feeds.bbci.co.uk/news/rss.xml?edition=int), [CNN World News](http://rss.cnn.com/rss/edition_world.rss)
    - RSS Guard: [Recent Commits to rssguard:master](https://github.com/martinrotter/rssguard/commits/master.atom)
    - Technology: [Linux Today](http://feeds.feedburner.com/linuxtoday/linux?format=xml), [TechCrunch](http://feeds.feedburner.com/techcrunch)

## Gmail API

This integration lets you download and read email messages stored in Gmail by making use of its [JSON API](https://developers.google.com/gmail/api). It also lets you send email messages. To make use of it follow the steps stated in [Gmail API documentation](https://developers.google.com/gmail/api/quickstart/js) or by configuring it directly in the [Google APIs Console](https://console.developers.google.com/apis/credentials):

1. Enable Gmail API if it isn't already.
2. Create a **Client ID** in the Google API console.
2. Create an **API key** in the Google API console.

Once you have this information you can configure your account in RSS Guard:

1. Go to **Accounts → Add New Account**. Select **Gmail**.
2. Enter required information:
  - Username: This will be your username _with_ the gmail.com part.
  - Application ID: As provided by Google.
  - Application Key: As provided by Google.
  - Only download newest X messages per feed
3. Click **Login** to start the authentication process. A browser window should open asking you to authorize RSS Guard to manipulate your Gmail account.
4. Once you've given permission, close the browser window.
5. In RSS Guard, click **OK**.

RSS Guard will start importing your messages at this point.
