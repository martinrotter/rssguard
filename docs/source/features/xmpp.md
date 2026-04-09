XMPP Plugin
===========
RSS Guard offers basic [`XMPP`](https://xmpp.org) support. The plugin supports these types of `XMPP` nodes:
* single-user chats
* multi-user chats
* [`PubSub`](https://xmpp.org/extensions/xep-0060.html) service nodes
* `PubSub` [`PEP`](https://xmpp.org/extensions/xep-0163.html) nodes¨

RSS Guard automatically fetches real-time messages for these nodes, including `ATOM` entries which are pushed for example via `AtomToPubsub`. RSS Guard can also fetch some archived messages.

```{note}
RSS Guard uses [qxmpp](https://invent.kde.org/libraries/qxmpp) library for this plugin. The library is has highly asynchronous API, therefore RSS Guard acts asynchronously in some cases as well. For example fetching archived messages is not instant but rather gradual.
````

```{warning}
`XMPP` support was added recently and is considered to be highly experimental.

Expect bugs.
```

## Settings
RSS Guard automatically [discovers](https://xmpp.org/extensions/xep-0030.html) `PubSub` services hosted on the same `XMPP` server which you are connected to. If you want to manually add more nodes, you have to use `Extra nodes` setting in the plugin configuration dialog.

These formats of extra nodes are supported:
* `something.url` (without `@`) are considered to be `PubSub` service directories, for example `news.movim.eu` is known directory.
* `something@something.url` (with `@`) are considered to be direct (single-user or multi-user) chats or `PubSub PEP` nodes. RSS Guard automatically finds out what kind of node you entered.