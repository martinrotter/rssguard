Gmail
=====
```{image} images/gmail.png
:alt: Gmail logo
:width: 64px
:align: right
```

The Gmail plugin presents e-mail messages inside RSS Guard's feed and article interface. It can synchronize message states and labels, display and download attachments, and send e-mail through the Gmail API.

This is not a general IMAP client. It is a focused Gmail integration for users who want to process mail alongside feeds.

## What the Account Contains
The account tree includes:

* Inbox
* Sent
* Drafts
* Spam
* user-created Gmail labels

Gmail labels appear in RSS Guard's **Labels** node. A message can have several synchronized labels even though it is shown as one article rather than being duplicated throughout the feed tree.

RSS Guard treats Gmail messages individually. It does not reproduce Gmail's conversation/thread view.

## Permissions
RSS Guard requests Gmail mailbox access so it can read messages, modify states and labels, download attachments, and send mail.

Only authorize the account when you trust the RSS Guard installation you are using. OAuth tokens are stored in the RSS Guard database and should not be shared in screenshots, logs or bug reports.

Google Workspace administrators can restrict third-party or unverified applications. If authorization is blocked for a managed account, the Workspace administrator may need to permit the OAuth application.

## Setup with Preconfigured Credentials
Official RSS Guard builds can include preconfigured Gmail OAuth credentials:

1. Open `Accounts -> Add account`.
2. Select `Gmail`.
3. Leave **Client ID** and **Client secret** empty.
4. Keep the default **Redirect URL**.
5. Select **Login**.
6. Sign in to Google in the browser and grant access.
7. Return to RSS Guard after the connection test succeeds.
8. Review the download options and save the account.

RSS Guard retrieves the Gmail address and fills in the username automatically. You may be asked to authorize once more when the newly saved account starts.

Preconfigured credentials have a shared Google API quota. Most users should try this method first. A private OAuth client can be useful when the shared quota is exhausted or when your RSS Guard build does not include credentials.

## Setup with Your Own OAuth Client
This setup is intended for advanced users and self-built packages.

1. Select **Get my credentials** in the Gmail account dialog.
2. Create or select a project in Google Cloud.
3. Enable the Gmail API.
4. Configure the Google Auth consent screen.
5. If the application remains in testing mode, add your Gmail address as a test user.
6. Create OAuth client credentials that permit the exact redirect URL displayed by RSS Guard.
7. Copy the client ID and client secret into RSS Guard.
8. Keep the redirect URL identical in Google Cloud and RSS Guard.
9. Select **Login** and complete authorization in the browser.

Google changes the Cloud Console interface periodically. Refer to Google's [Gmail API quickstart](https://developers.google.com/workspace/gmail/api/quickstart/js) and [OAuth consent-screen documentation](https://support.google.com/cloud/answer/13461325) when the labels in the console differ.

```{note}
An unverified personal OAuth project may display a warning. Projects in testing mode only work for accounts included in their test-user list. For an external project in **Testing** status, Google normally expires authorization and its refresh token after seven days when Gmail access is requested. See Google's [OAuth documentation](https://developers.google.com/identity/protocols/oauth2#expiration).
```

## Download Options
### Download Unread Messages Only
Enable **Download unread articles only** to avoid importing the normal read-message history.

This keeps the local database smaller, but read messages that are not already stored locally will generally be omitted. Starred messages needed for state synchronization can still be downloaded.

### Message Limit
**Only download newest X articles per feed** limits the number of messages requested for each Gmail mailbox item or label.

The default is 100. Increase it carefully when you want more history, because each downloaded message includes its body and metadata.

Gmail synchronization is incremental: RSS Guard compares remote message identifiers and states with the local database and downloads missing or changed messages.

## Reading Mail
Gmail uses a dedicated message preview that displays:

* sender
* recipients
* subject
* HTML or plain-text message body
* attachment menu
* Reply and Forward actions

Attachments are not downloaded automatically with every message. Open **Attachments** in the preview and choose a file to download it.

Some unusually structured MIME messages may not render exactly like Gmail's web interface. The original message can be opened in Gmail through its article URL.

## Sending Mail
Open the Gmail account menu and choose **Write new e-mail message**. The editor supports:

* rich-text message contents
* subject
* To recipients
* Cc and Bcc recipients
* Reply-To
* recipient suggestions from previously downloaded messages

You can also reply to or forward a selected message from the Gmail preview or article context menu.

The current composer does not attach files to outgoing messages.

## Synchronized Changes
RSS Guard synchronizes:

* read and unread state
* Gmail **Starred** state as RSS Guard importance
* user-created Gmail label assignments
* removal of user-created Gmail labels from messages

Label definitions should be created, renamed and deleted in Gmail. Synchronize the account afterward to refresh them in RSS Guard.

RSS Guard does not expose Gmail mailbox management as normal feed and folder editing. System mailbox items and Gmail labels come from the service.

## Proxy Settings
The account dialog includes RSS Guard's common proxy settings. They apply to Gmail API calls, message synchronization, sending mail and attachment downloads.

The external browser used for Google authorization follows the browser's own network configuration.

## Supported Features
The Gmail plugin supports:

* OAuth authentication
* incremental two-way state synchronization
* synchronized Gmail labels
* Inbox, Sent, Drafts and Spam views
* HTML and plain-text messages
* on-demand attachment downloads
* composing rich-text e-mail
* reply and forward
* unread-only downloading
* per-item message limits
* account-specific proxy settings

## Limitations
* Only Gmail accounts are supported; this is not a generic IMAP or SMTP plugin.
* Messages are displayed individually, without Gmail conversation grouping.
* Gmail labels and system mailbox items are managed by Gmail.
* The outgoing composer does not support file attachments.
* Unread-only mode intentionally creates an incomplete local history.
* OAuth access can be restricted by Google Workspace policy or by an unverified project's test-user settings.
