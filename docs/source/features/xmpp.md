XMPP Plugin
===========
RSS Guard offers basic [`XMPP`](https://xmpp.org) support. The plugin can discover and work with these types of `XMPP` nodes:
* single-user chats
* multi-user chats
* [`PubSub`](https://xmpp.org/extensions/xep-0060.html) service nodes
* `PubSub` [`PEP`](https://xmpp.org/extensions/xep-0163.html) nodes

In the UI, these are grouped into four categories:
* `Single-user chats`
* `Multi-user chats`
* `PubSub services`
* `PubSub PEPs`

RSS Guard can receive real-time messages for these nodes. For `PubSub` and `PEP`, this includes `ATOM` entries pushed by the server, for example via `AtomToPubsub`.

```{note}
RSS Guard uses the [qxmpp](https://invent.kde.org/libraries/qxmpp) library for this plugin. The library has a highly asynchronous API; therefore RSS Guard also acts asynchronously in some cases. For example, service discovery and archived-message fetching are not instant, but rather gradual.
```

```{warning}
`XMPP` support is still considered experimental.

Expect bugs and rough edges.
```

## Account Setup
An XMPP account in RSS Guard uses these fields:
* `JID` - your full XMPP identifier, for example `user@example.org`
* `Password`
* `Host` - usually the server domain
* `Extra nodes` - optional additional JIDs or service IDs to include in discovery

When you type a `JID`, RSS Guard automatically tries to derive the host from its domain part.

## Discovery
RSS Guard discovers XMPP content from multiple sources:
* the connected server's `disco#items` results
* user-defined `Extra nodes`
* one built-in default extra node: `news.movim.eu`

This means the discovered tree is not limited only to items explicitly hosted on your own account's domain.

For `PubSub` and `PEP`, RSS Guard creates feeds from **subscriptions** returned by the server. In other words, it does not arbitrarily import every possible node, only the ones you are subscribed to.

Feeds are discovered and synchronized from the server side. They are not added manually in the same way as regular RSS feeds.

## `Extra nodes`
`Extra nodes` accepts one item per line. These formats are supported:
* `news.movim.eu`
  A PubSub service JID.
* `user@example.org`
  An account JID. This may be recognized either as a direct chat target or as a PEP-capable account, depending on what the server reports during discovery.
* `room@conference.example.org`
  A multi-user chat room JID.

In practical terms, RSS Guard probes each extra node and classifies it according to the server's discovery information.

## Fetching Behavior
RSS Guard uses different retrieval strategies for different node types:

### Single-user chats and multi-user chats
Chat-like feeds are fetched from the server archive using `MAM` when you fetch them manually or when RSS Guard refreshes them.

Real-time handling differs slightly:
* single-user chats are received from normal incoming XMPP chat messages
* multi-user chats are received after RSS Guard joins the saved rooms on connect

### PubSub services and PubSub PEPs
PubSub-like feeds are fetched by requesting item IDs and then downloading the missing items.

Currently, RSS Guard downloads up to 10 unseen PubSub items per fetch pass. Real-time PubSub events are handled separately and can deliver new articles immediately.

## Connection Behavior
After connecting successfully:
* RSS Guard can show connection status notifications
* if the account has no saved subtree yet, it triggers discovery
* if feeds already exist, it reconnects and joins saved multi-user chat rooms

RSS Guard also tracks supported XEPs reported by the server and shows them in the account tooltip.

## Current Limitations
Some current implementation details are worth keeping in mind:
* Discovery depends on what the server exposes plus any `Extra nodes` you configured.
* PubSub import is subscription-based, not full node browsing.
* PubSub fetch currently downloads at most 10 new items per pass.
* Multi-user chat rooms are joined automatically only for rooms already present in the saved account tree.
* XMPP feeds cannot be created manually through the generic "add feed" workflow.
