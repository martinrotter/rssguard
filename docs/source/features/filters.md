Article Filtering
=================
Sometimes you need to automatically tweak the incoming article - mark it starred, remove ads from its contents or simply reject it. That's where article filtering feature comes in.

## `Article filters` dialog
The dialog seen below offers you a way of managing your article filters. You can assign single filter to multiple feeds across all accounts.

`Test` button tests selected filter against existing messages. `Process checked feeds` runs the filter against existing messages from checked feeds - in this mode all modifications made by the filter are saved to existing messages.

<img alt="alt-img" src="images/filters-dialog.png" width="600px">

## Writing article filter
Article filters are small scripts which are executed automatically when articles/feeds are downloaded. Article filters are `JavaScript` snippets which must provide function with prototype:

```js
function filterMessage() { }
```

The function must return values which belong to enumeration [`FilteringAction`](#filteringaction-enum).

Supported set of built-in "standard library" adheres to [ECMA-262](https://ecma-international.org/publications-and-standards/standards/ecma-262).

Each article is accessible in your script via global variable/property named `msg` of type `MessageObject`, see [this file](https://github.com/martinrotter/rssguard/blob/master/src/librssguard/core/messageobject.h) for the declaration. Some properties are writeable, allowing you to change contents of the article before it is written to RSS Guard DB. You can mark article important, change its description, perhaps change author name or even assign some [label](labels) to it!!!

```{note}
Some attributes (`read/unread/starred` states) are synchronized back to your account's server - so you can for example mark some articles as starred and the change will be propagated back to TT-RSS server if you use TT-RSS.
```

```{attention}
A special [placeholders](userdata.md#data-placeholder) can be used in article filters.
```

There is also a special variable named `utils`. This variable is of `FilterUtils` type. It offers some useful utility functions for you to use in your filters.

[Labels](labels) assigned to articles are visible to your filters. You can, therefore, execute actions in your filtering script, based on which labels are assigned to the article. The property is called `assignedLabels` and is an array of the `Label` objects.

Passed article also offers a special function:

```js
Boolean MessageObject.isAlreadyInDatabase(DuplicateCheck)
```

which allows you to perform runtime check for existence of the article in RSS Guard database. Parameter is the value from enumeration `DuplicateCheck`. It specifies how exactly the article should match.

For example, if you want to check if there is already another article by the same author in a database, you should call `msg.isAlreadyInDatabase(MessageObject.SameAuthor)`.  
The values of enumeration can be combined in a single call with the [bitwise OR](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Bitwise_OR) (`|`) operator, like this:

```js
msg.isAlreadyInDatabase(MessageObject.SameAuthor | MessageObject.SameUrl)
```

## Class Reference Documentation

Here is the reference documentation of types available for your filtering scripts.

### `MessageObject` class
| Type      | Name(Parameters)              | Return value  | Read-only  | Synchronized  | Description
| :---      | :---                          | :---          | :---:      | :---:         | ---
| Property  | `assignedLabels`              | `Array<Label>`| ✅         | ✅            | List of labels assigned to the message/article.
| Property  | `availableLabels`             | `Array<Label>`| ✅         | ❌            | List of labels which are currently available and can be assigned to the message. Available in RSS Guard 3.8.1+.
| Property  | `feedCustomId`                | `String`      | ✅         | ❌            | Service-specific ID of the feed which this message belongs to.
| Property  | `accountId`                   | `Number`      | ✅         | ❌            | RSS Guard's ID of the account activated in the program. This property is highly advanced, and you probably do not need to use it at all.
| Property  | `id`                          | `Number`      | ✅         | ❌            | ID assigned to the message in RSS Guard local database.
| Property  | `customId`                    | `String`      | ❌         | ❌            | ID of the message as provided by the remote service or feed file.
| Property  | `title`                       | `String`      | ❌         | ❌            | The message title.
| Property  | `url`                         | `String`      | ❌         | ❌            | The message URL.
| Property  | `author`                      | `String`      | ❌         | ❌            | Author of the message.
| Property  | `contents`                    | `String`      | ❌         | ❌            | Contents of the message.
| Property  | `rawContents`                 | `String`      | ❌         | ❌            | This is the RAW contents of the message obtained from remote service/feed. A raw XML or JSON element data. This attribute has the value only if `runningFilterWhenFetching` returns `true`. In other words, this attribute is not persistently stored in the RSS Guard's DB. Also, this attribute is artificially filled in with ATOM-like data when testing the filter.
| Property  | `score`                       | `Number`      | ❌         | ❌            | Arbitrary number in range \<0.0, 100.0\>. You can use this number to sort messages in a custom fashion as this attribute also has its own column in articles list.
| Property  | `hasEnclosures`               | `Boolean`     | ✅         | ❌            | Returns `true` if the article has at least one enclosure/attachment. Otherwise returns `false`.
| Property  | `created`                     | `Date`        | ❌         | ❌            | Date/time of the message.
| Property  | `isRead`                      | `Boolean`     | ❌         | ✅            | Is message read?
| Property  | `isImportant`                 | `Boolean`     | ❌         | ✅            | Is message important?
| Property  | `isDeleted`                   | `Boolean`     | ❌         | ❌            | Is message placed in recycle bin?
| Method    | `addEnclosure(String url, String mime_type)` | `void` | ❌         | ❌            | Adds multimedia attachment to the article.
| Method    | `isAlreadyInDatabase(DuplicateCheck criteria)` | `Boolean` | ❌         | ❌            | Allows you to check if message is already stored in the RSS Guard's DB. See possible parameters.
| Method    | `findLabelId(String label_name)`         | `String`     | ❌         | ❌            | If you enter the label name, method returns label's `customId` which then can be used in `assignLabel()` and `deassignLabel` methods.
| Method    | `assignLabel(String label_id)`         | `Boolean`     | ❌         | ❌            | Assigns label to the message. The `String` value is the `customId` property of `Label` type. See its API reference for relevant info.
| Method    | `deassignLabel(String label_id)`       | `Boolean`     | ❌         | ❌            | Removes label from the message. The `String` value is the `customId` property of `Label` type. See its API reference for relevant info.
| Method    | `createLabelId(String title, String color_hex_code)`       | `String`     | ❌         | ❌            | Creates the label with given name and given color hex code in form `#AABBCC`. Color can be omitted in which case auto-generated color is used.
| Method    | `addEnclosure(String url, String mime_type)`       | `void`     | ❌         | ❌            | Appends new enclosure/attachment with given URL and MIME type to the article.
| Property  | `runningFilterWhenFetching`   | `Boolean`     | ✅         | ❌            | Returns `true` if message filter is applied when message is fetched. Returns `false` if filter is applied manually, for example from `Article filters` window.

### `Label` class
| Type      | Name          | Return value  | Read-only | Description
| :---      | :---          | :---          | :---:     | ---
| Property  | `title`       | `String`      | ✅        | Label name.
| Property  | `customId`    | `String`      | ✅        | Service-specific ID of the label. The ID is used as a unique identifier for label. It is useful if you want to assign/unassign the message label.
| Property  | `color`       | `Color`       | ✅        | Label color. See the type `color` documentation in [Qt docs](https://doc.qt.io/qt-5/qml-color.html).

### `FilteringAction` enum
| Enumerant name    | Integer value | Description
| :---              | :---          | ---
| `Accept`          | 1             | Message is accepted and will be added or updated in DB.
| `Ignore`          | 2             | Message is ignored and will **NOT** be added or updated in DB. Already existing message will not be purged from DB.
| `Purge`           | 4             | Existing message is purged from the DB completely. Behaves like `Ignore` when there is a new incoming message.

```{attention}
The `MessageObject` attributes are synchronized with service even if you return `Purge` or `Ignore`. In other words, even if the filter ignores the article, you can still tweak its properties, and they will be synchronized back to your server.
```

### `DuplicateCheck` enum
| Enumerant name    | Integer value | Description
| :---              | :---          | ---
| `SameTitle`       | 1             | Check if message has same title as some another messages.
| `SameUrl`         | 2             | Check if message has same URL as some another messages.
| `SameAuthor`      | 4             | Check if message has same author as some another messages.
| `SameDateCreated` | 8             | Check if message has same date of creation as some another messages.
| `AllFeedsSameAccount` | 16        | Perform the check across all feeds from your account, not just "current" feed.
| `SameCustomId`    | 32            | Check if message with same custom ID exists in RSS Guard DB.

### `utils` object
| Type      | Name(Parameter)           | Return value  | How to call                               | Description
| :---      | :---                      | :---          | :---                                      | ---
| Method    | `hostname()`              | `String`      | `utils.hostname()`                        | Returns name of your PC.
| Method    | `fromXmlToJson(String xml_string)`   | `String`      | `utils.fromXmlToJson('<h1>hello</h1>')`   | Converts XML string into JSON.
| Method    | `parseDateTime(String date_time)`   | `Date`        | `utils.parseDateTime('2020-02-24T08:00:00')`  | Converts textual date/time representation into proper `Date` object.
| Method    | `runExecutableGetOutput(String exec, String[] params)`   | `String`        | `utils.runExecutableGetOutput('cmd.exe', ['/c', 'dir'])`  | Launches external executable with optional parameters, reads its standard output, and returns the output when executable finishes.

## Examples
Accept only messages/articles with title containing "Series Name" or "Another series" in it (whitelist):
```js
var whitelist = [
  'Series Name', 'Another series'
];
function filterMessage() {
  if (whitelist.some(i => msg.title.indexOf(i) != -1)) {
    return MessageObject.Accept;
  } else {
    return MessageObject.Ignore;
  }
}
```

Accept only messages/articles with title NOT containing "Other Series Name" or "Some other title" in it (blacklist):
```js
var blacklist = [
  'Other Series Name', 'Some other title'
];
function filterMessage() {
  if (blacklist.some(i => msg.title.indexOf(i) != -1)) {
    return MessageObject.Ignore;
  } else {
    return MessageObject.Accept;
  }
}
```

Accept only messages/articles from "Bob", while also mark them important:
```js
function filterMessage() {
  if (msg.author == "Bob") {
    msg.isImportant = true;
    return MessageObject.Accept;
  }
  else {
    return MessageObject.Ignore;
  }
}
```

Replace all "dogs" with "cats"!
```js
function filterMessage() {
  msg.title = msg.title.replace("dogs", "cats");
  return MessageObject.Accept;
}
```

Use published element instead of updated element (for `ATOM` entries only):
```js
function filterMessage() {
  // Read raw contents of message and
  // convert to JSON.
  json = utils.fromXmlToJson(msg.rawContents);
  jsonObj = JSON.parse(json)

  // Read published date and parse it.
  publishedDate = jsonObj.entry.published.__text;
  parsedDate = utils.parseDateTime(publishedDate);

  // Set new date/time for message and
  // proceed.
  msg.created = parsedDate;
  return MessageObject.Accept;
}
```

Dump RAW data of each message to RSS Guard debug output:
```js
function filterMessage() {
  console.log(msg.rawContents);
  return MessageObject.Accept;
}
```

When running the above script for Tiny Tiny RSS, it produces the following debug output:
```
...
time="    34.360" type="debug" -> feed-downloader: Hooking message took 4 microseconds.
time="    34.361" type="debug" -> {"always_display_attachments":false,"attachments":[],"author":"Aleš Kapica","comments_count":0,"comments_link":"","content":"<p>\nNaposledy jsem psal o čuňačení v MediaWiki asi před půl rokem, kdy jsem chtěl upozornit na to, že jsem přepracoval svoji původní šablonu Images tak, aby bylo možné používat výřezy z obrázků a stránek generovaných z DjVu a PDF dokumentů. Blogpost nebyl nijak extra hodnocen, takže mě vcelku nepřekvapuje, jak se do hlavní vývojové větve MediaWiki dostávají čím dál větší prasečiny.\n</p>","feed_id":"5903","feed_title":"abclinuxu - blogy","flavor_image":"","flavor_stream":"","guid":"{\"ver\":2,\"uid\":\"52\",\"hash\":\"SHA1:5b49e4d8f612984889ba25e7834e80604c795ff8\"}","id":6958843,"is_updated":false,"labels":[],"lang":"","link":"http://www.abclinuxu.cz/blog/kenyho_stesky/2021/1/cunacime-v-mediawiki-responzivni-obsah-ii","marked":false,"note":null,"published":false,"score":0,"tags":[""],"title":"Čuňačíme v MediaWiki - responzivní obsah II.","unread":true,"updated":1610044674}
time="    34.361" type="debug" -> feed-downloader: Running filter script, it took 348 microseconds.
time="    34.361" type="debug" -> feed-downloader: Hooking message took 4 microseconds.
time="    34.361" type="debug" -> {"always_display_attachments":false,"attachments":[],"author":"kol-ouch","comments_count":0,"comments_link":"","content":"Ahoj, 1. 6. se blíží, tak začínám řešit co s bambilionem fotek na google photos. \n<p class=\"separator\"></p>\nZa sebe můžu říct, že gp mi vyhovují - ne snad úplně tím, že jsou zadarmo, ale hlavně způsobem práce s fotkami, možnostmi vyhledávání v nich podle obsahu, vykopírování textu z nich, provázaností s mapami, recenzemi, možnostmi sdílení, automatickým seskupováním a podobně.","feed_id":"5903","feed_title":"abclinuxu - blogy","flavor_image":"","flavor_stream":"","guid":"{\"ver\":2,\"uid\":\"52\",\"hash\":\"SHA1:1277107408b159882b95ca7151a0ec0160a3971a\"}","id":6939327,"is_updated":false,"labels":[],"lang":"","link":"http://www.abclinuxu.cz/blog/Co_to_je/2021/1/kam-s-fotkama","marked":false,"note":null,"published":false,"score":0,"tags":[""],"title":"Kam s fotkama?","unread":true,"updated":1609750800}
...
```

For `RSS 2.0` message, the result might look as follows:
```
...
time="     3.568" type="debug" -> feed-downloader: Hooking message took 6 microseconds.
time="     3.568" type="debug" -> <item>
<title><![CDATA[Man Utd's Cavani 'not comfortable' in England, says father]]></title>
<description><![CDATA[Manchester United striker Edinson Cavani "does not feel comfortable" and could move back to his native South America, his father said.]]></description>
<link>https://www.bbc.co.uk/sport/football/56341983</link>
<guid isPermaLink="true">https://www.bbc.co.uk/sport/football/56341983</guid>
<pubDate>Tue, 09 Mar 2021 23:46:03 GMT</pubDate>
</item>

time="     3.568" type="debug" -> feed-downloader: Running filter script, it took 416 microseconds.
...
```

Write details of available labels and assign the first label to the message:
```js
function filterMessage() {
  console.log('Number of assigned labels: ' + msg.assignedLabels.length);
  console.log('Number of available labels: ' + msg.availableLabels.length);

  var i;
  for (i = 0; i < msg.availableLabels.length; i++) {
    var lbl = msg.availableLabels[i];

    console.log('Available label:');
    console.log('  Title: \'' + lbl.title + '\' ID: \'' + lbl.customId + '\'');
  }

  if (msg.availableLabels.length > 0) {
    console.log('Assigning first label to message...');
    msg.assignLabel(msg.availableLabels[0].customId);

    console.log('Number of assigned labels ' + msg.assignedLabels.length);
  }

  console.log();
  return MessageObject.Accept;
}
```

Make sure that you receive only one message with particular URL across all your feeds, plugin/account-wide. All other messages with the same URL are subsequently ignored:
```js
function filterMessage() {
  if (msg.isAlreadyInDatabase(MessageObject.SameUrl | MessageObject.AllFeedsSameAccount)) {
    return MessageObject.Ignore;
  }
  else {
    return MessageObject.Accept;
  }
}
```

Have multiple separate conditions and assign article to proper label according to condition:
```js
// Conditions go here, condition is matched if title contains the condition phrase.
var conditions = [
  'notification',
  'tired',
  'upgrade'
];

// Names of your labels go here.
// Count of names must be same as count of conditions above.
var labels = [
  'Notifications',
  'Tired stuff',
  'Upgrades'
];

function filterMessage() {
  var target_idx = conditions.findIndex(i => msg.title.indexOf(i) >= 0);

  if (target_idx >= 0) {
    var target_lbl = msg.findLabelId(labels[target_idx]);

    msg.assignLabel(target_lbl);
  }

  return MessageObject.Accept;
}
```

Remove "ads" from messages received from Inoreader. Method simply removes `div` which contains the advertisement:
```js
function filterMessage() {
  msg.contents = msg.contents.replace(/<div>\s*Ads[\S\s]+Remove<\/a>[\S\s]+adv\/www\/delivery[\S\s]+?<\/div>/im, '');

  return MessageObject.Accept;
}
```
