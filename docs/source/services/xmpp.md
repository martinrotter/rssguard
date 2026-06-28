XMPP
====
```{image} images/xmpp.png
:alt: XMPP logo
:width: 64px
:align: right
```

The XMPP plugin presents selected chats and PubSub subscriptions as article feeds in RSS Guard. It can receive chat messages and Atom entries in real time and can retrieve available history from the XMPP server.

This is a specialized, experimental integration rather than a complete chat client.

```{note}
The XMPP plugin is available only in RSS Guard builds based on Qt 6, and a packager may choose not to include it.
```

RSS Guard uses the [QXmpp](https://invent.kde.org/libraries/qxmpp) library for the connection and supported XMPP extensions. Discovery, archive retrieval and PubSub requests are asynchronous, so their results can appear a short time after an action is started.

## Supported XMPP Sources
Discovered items are organized into four folders:

* **Single-user chats**
* **Multi-user chats**
* **PubSub services**
* **PubSub PEPs**

[PubSub](https://xmpp.org/extensions/xep-0060.html) services can distribute feed-like items through XMPP. [Personal Eventing Protocol](https://xmpp.org/extensions/xep-0163.html), usually called PEP, provides similar publishing under an individual account.

For PubSub and PEP nodes, RSS Guard expects Atom entries. A service such as AtomToPubsub can publish ordinary feed entries this way.

## Add the Account
You need an existing XMPP account and its full Jabber ID:

1. Open `Accounts -> Add account`.
2. Select `XMPP`.
3. Enter the full **JID**, for example `user@example.org`.
4. Enter the account password.
5. Check **Host**. RSS Guard derives it from the JID while you type.
6. Add any known services, contacts or rooms under **Extra nodes**, one address per line.
7. Select **Test setup**.
8. Review the common proxy settings when needed.
9. Save the account after the connection test succeeds.

The host is normally the XMPP domain, such as `example.org`, rather than a web URL. Only change the automatically derived value when your provider requires a different connection domain.

```{warning}
Do not include your XMPP password in screenshots, logs or bug reports.
```

## Extra Nodes
RSS Guard combines the server's discovery results with the addresses listed under **Extra nodes**. Add one address per line:

```text
news.movim.eu
person@example.org
room@conference.example.org
```

An address can represent:

* a PubSub service, such as `news.movim.eu`
* an XMPP account with PEP subscriptions
* a person for a single-user chat
* a multi-user chat room

RSS Guard queries each address to determine its type. The server must expose enough service-discovery information for the item to be recognized.

New accounts initially include `news.movim.eu` as an example PubSub service. You can remove it when it is not useful.

## Discovery
When the account connects for the first time, RSS Guard:

1. requests the XMPP server's advertised services
2. adds the configured extra nodes
3. identifies supported chat, room, PubSub and PEP addresses
4. requests the current PubSub subscriptions
5. creates the corresponding RSS Guard folders and feeds

Discovery is asynchronous and may take several seconds. Some nodes can be omitted when a server does not answer in time or does not advertise the expected capabilities.

For PubSub and PEP, RSS Guard imports nodes to which the account is already subscribed. It does not browse every public node hosted by a service.

Use the normal account refresh action after changing **Extra nodes** or server-side subscriptions. The account menu also provides **Reconnect** when the connection needs to be restarted.

## Receiving Articles
### Single-User Chats
An incoming one-to-one chat message is added to the matching RSS Guard feed in real time. When the server supports message archives, refreshing the feed requests archived messages too.

The sender's bare JID is used as the author. The message subject becomes the article title when available; otherwise RSS Guard derives a short title from the message body.

### Multi-User Chats
RSS Guard joins saved rooms after connecting. New group-chat messages then arrive in real time, and refreshing a room requests available archived messages.

RSS Guard joins with a generated nickname based on its application name and your JID username. Password-protected rooms and interactive room setup are not supported.

### PubSub Services and PEP
RSS Guard listens for pushed PubSub events while connected. When notified about a new item, it requests the complete item and parses its Atom payload into an article.

Refreshing a PubSub feed compares remote item identifiers with locally stored articles and downloads a small group of missing items.

### Manual Asynchronous Fetch
The context menu of an XMPP feed contains **Trigger async fetch**. This explicitly requests archived chat messages or missing PubSub items, depending on the selected feed type. Retrieved items are inserted after the asynchronous request finishes.

## How the Integration Works
### Service Classification
RSS Guard uses XMPP service discovery to inspect the identities and features advertised by each server-provided or user-supplied address. It recognizes:

* a PubSub service from a `pubsub/service` identity
* a PEP-capable account from a `pubsub/pep` identity
* a multi-user chat from a `conference/text` identity with MUC support
* a single-user chat target from a registered account identity

This is why an apparently valid JID can still be absent after discovery: RSS Guard does not classify an address solely from its shape.

For PubSub services and PEP accounts, RSS Guard asks the server for the current subscription list. Each reported node becomes a feed. PEP microblog nodes are accepted only from individual account JIDs.

Discovery has an idle timeout so that one unresponsive service does not leave synchronization running forever. The resulting tree can therefore be partial when some services respond too slowly.

### Archived Chat Retrieval
Chat history is requested through the server's Message Archive Management support:

* a single-user feed requests archived messages exchanged with that contact
* a multi-user feed requests the archive of that room

RSS Guard accepts chat and group-chat messages that have a body, title and stable message identifier. XHTML content is preferred when supplied; otherwise the plain message body is used. Server timestamps are retained, while messages without a timestamp use their arrival time.

Archive availability, retention and access permissions are controlled by the XMPP server. A successful account connection does not guarantee that historical messages can be retrieved.

### PubSub Retrieval
When a PubSub feed is refreshed, RSS Guard first requests its item identifiers and compares them with read, unread and starred article identifiers already stored locally. It then requests up to ten missing items in that fetch and parses their Atom payloads.

For a real-time PubSub event, RSS Guard receives the event notification, requests the referenced complete item from the service, parses the Atom entry and schedules the matching feed for an article update.

If an event refers to a node that is not present in the current RSS Guard tree, the article cannot be stored. Refreshing XMPP discovery can restore a missing subscribed node.

### Real-Time Chat Routing
Normal incoming chat messages are routed to a saved single-user feed by the sender's bare JID. Multi-user messages are handled by the room object after RSS Guard joins that room. PubSub events are handled separately by the PubSub manager.

Consequently, an incoming message is stored only when a matching discovered feed already exists.

## Local Article State
Messages received through XMPP become normal RSS Guard articles. You can mark them read, star them, label them locally, filter them and search them.

These RSS Guard states are not sent back through XMPP. The plugin does not provide read receipts, synchronized stars, synchronized labels or chat-state synchronization.

## Removing Discovered Items
An XMPP feed can be deleted from the RSS Guard tree, but deletion is local only. It does not:

* unsubscribe from a PubSub node
* leave or destroy a server-side room
* remove a contact

If the item is still discoverable or remains in **Extra nodes**, it can appear again after rediscovery. Remove the extra-node entry or change the subscription with a full XMPP client when you want it to stay absent.

## Connection and Proxy Behavior
RSS Guard keeps the XMPP connection open to receive real-time events. It displays notifications when the account connects, disconnects or encounters an error.

The account tooltip shows the current connection state and the XMPP extension protocols reported by the server.

The common account proxy setting is passed to the XMPP connection. Whether a particular proxy type works also depends on the server, network and Qt XMPP support.

## Supported Features
The XMPP plugin supports:

* XMPP account authentication
* service discovery
* user-supplied discovery addresses
* single-user chat messages
* multi-user chat rooms
* PubSub subscriptions
* PEP subscriptions
* real-time incoming messages and PubSub events
* archived chat retrieval when supported by the server
* retrieval of missing PubSub items
* account-specific proxy settings

## Limitations
* The integration is experimental.
* RSS Guard receives messages but does not provide a chat composer.
* End-to-end encrypted chat, including OMEMO, is not supported by this integration.
* Chat read state, stars and labels remain local to RSS Guard.
* Feeds cannot be added through the normal add-feed dialog.
* PubSub nodes must already be subscribed on the server.
* Discovery depends on server capabilities and can time out.
* Password-protected room joining and interactive room configuration are not supported.
* Deleting an item does not alter its server-side subscription or contact.

For general protocol information, see [XMPP.org](https://xmpp.org/).
