# Message filtering
RSS Guard supports _automagic_ message filtering. The filtering system is automatically triggered when new messages for each feed are downloaded. User can write scripts which perform filtering decisions. [**JavaScript with ECMA standard**](http://www.ecma-international.org/publications/standards/Ecma-262.htm) is supported.

## Message downloading/filtering workflow
```
foreach (feed in feeds_to_update) do
  messages = download_messages(feed)
  filtered_messages = filter_messages(messages)
  save_messages_to_database(filtered_messages)
```
As you can see, RSS Guard processes all feeds scheduled for message downloading one by one; downloading new messages, feeding them to filtering system and then saving all approved messages to RSS Guard's database.

## Writing message filter
Message filter consists of arbitrary JavaScript code which must provide function with prototype

```js
function filterMessage() { }
```

This function must be fast and must return values which belong to enumeration `FilteringAction` from this [file](https://github.com/martinrotter/rssguard/blob/master/src/librssguard/core/messageobject.h). You can you use either direct numerical value of each enumerant, for example `2` or you can use self-descriptive enumerant name, for example `MessageObject.Ignore`. There are also names `MSG_ACCEPT` and `MSG_IGNORE` as aliases for `MessageObject.Accept` and `MessageObject.Ignore`.

Each message is accessible in your script via global variable named `msg` of type `MessageObject`, see this [file](https://github.com/martinrotter/rssguard/blob/master/src/librssguard/core/messageobject.h) for the declaration. Some properties are writable, allowing you to change contents of the message before it is written to DB. You can mark message important, parse its description or perhaps change author name or even assign some label to it!!!

You can use [special placeholders](Documentation.md#data-placeholder) within message filter.

Also, there is a special variable named `utils`. This variable is of type `FilterUtils` and offers some useful utility [functions](#utils-object) for you to use in your filters.

RSS Guard also offers list of labels assigned to each message. You can therefore do actions in your filtering script based on which labels are assigned to the message. The property is called `assignedLabels` and is array of `Label` objects. If you change assigned labels to the message, then the change will be eventually synchronized back to server if respective plugin supports it.

Passed message also offers special function
```js
Boolean MessageObject.isDuplicateWithAttribute(DuplicationAttributeCheck)
```

which allows you to perform runtime check for existence of the message in RSS Guard's database. The parameter is integer value from enumeration `DuplicationAttributeCheck` from this [file](https://github.com/martinrotter/rssguard/blob/master/src/librssguard/core/messageobject.h) and specifies how exactly you want to determine if given message is "duplicate". Again, you can use direct integer values or enumerant names.

For example if you want to check if there is already another message with same author in database, then you call `msg.isDuplicateWithAttribute(MessageObject.SameAuthor)`. Values of the enumeration can be combined via bitwise `|` operation in single call, for example like this: `msg.isDuplicateWithAttribute(MessageObject.SameAuthor | MessageObject.SameUrl)`.

## API reference
Here is the reference of methods and properties of some types available in your filtering scipts.

### `MessageObject` class
| Property/method | Description |
|---|---|
| `Array<Label> assignedLabels` | `READ-ONLY` List of labels assigned to the message. |
| `Array<Label> availableLabels` | `READ-ONLY` List of labels which are currently available and can be assigned to the message. Available in RSS Guard 3.8.1+. |
| `String feedCustomId` | `READ-ONLY` Service-specific ID of the feed which this message belongs to. |
| `Number accountId` | `READ-ONLY` RSS Guard's ID of the account activated in the program. This property is highly advanced and you probably do not need to use it at all. |
| `String title` | Title of the message. |
| `String url` | URL of the message. |
| `String author` | Author of the message. |
| `String contents` | Contents of the message. |
| `String rawContents` | This is RAW contents of the message as it was obtained from remote service/feed. You can expect raw `XML` or `JSON` element data here. Note that this attribute has some value only if `alreadyStoredInDb` returns `false`. In other words, this attribute is not persistently stored inside RSS Guard's DB. |
| `Number score` | Arbitrary number in range <0.0, 100.0>. You can use this number to sort messages in a custom fashion as this attribute also has its own column in messages list. |
| `Date created` | Date/time of the message. |
| `Boolean isRead` | Is message read? |
| `Boolean isImportant` | Is message important? |
| `Boolean isDeleted` | Is message placed in recycle bin? |
| `Boolean isDuplicateWithAttribute(DuplicationAttributeCheck)` | Allows you to test if this particular message is already stored in RSS Guard's DB. |
| `Boolean assignLabel(String)` | Assigns label to this message. The passed `String` value is the `customId` property of `Label` type. See its API reference for relevant info. |
| `Boolean deassignLabel(String)` | Removes label from this message. The passed `String` value is the `customId` property of `Label` type. See its API reference for relevant info. |
| `Boolean alreadyStoredInDb` | `READ-ONLY` Returns true if this message is already stored in DB. This function is the way to check if the filter is being run automatically for newly downloaded messages or manually for already existing messages.

### `Label` class
| Property/method | Description |
|---|---|
| `String title` | `READ-ONLY` Label title. |
| `String customId` | `READ-ONLY` Service-specific ID of this label. This ID is used as unique identifier for the label and is particularly useful if you want to (de)assign label to/from message. |
| `color color` | `READ-ONLY` Label color. Note that type `color` has its documentation [here](https://doc.qt.io/qt-5/qml-color.html). |

### `FilteringAction` enum
| Enumerant name | Integer value | Description |
|---|---|---|
| `Accept` | 1 | Message is accepted and will be added to DB or updated in DB. |
| `Ignore` | 2 | Message is ignored and will be **NOT** added to DB or updated in DB, but is not purged from DB if already exists. |
| `Purge` | 4 | Existing message is purged from the DB completely. |

Note that `MessageObject` attributes which can be synchronized back to service are synchronized even if you return `Purge` or `Ignore`. In other words: even if you filter ignores the message you can still tweak its properties which will get synchronized back to your server.

### `DuplicationAttributeCheck` enum
| Enumerant name | Integer value | Description |
|---|---|---|
| `SameTitle` | 1 | Check if message has same title as some another messages. |
| `SameUrl` | 2 | Check if message has same URL as some another messages. |
| `SameAuthor` | 4 | Check if message has same author as some another messages. |
| `SameDateCreated` | 8 | Check if message has same date of creation as some another messages. |
| `AllFeedsSameAccount` | 16 | Perform the check across all feeds from your account, not just "current" feed. |

## `utils` object
| Method | How to call | Description |
|---|---|---|
| `String hostname()` | `utils.hostname()` | Returns name of your PC. |
| `String fromXmlToJson(String)` | `utils.fromXmlToJson('<h1>hello</h1>')` | Converts `XML` string into `JSON`. |

## Examples
Accept only messages from "Bob" while also marking them important.
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

Replace all dogs with cats!
```js
function filterMessage() {
  msg.title = msg.title.replace("dogs", "cats");
  return MessageObject.Accept;
}
```

Dump RAW data of each message to RSS Guard's [debug output](Documentation.md#generating-debug-log-file).
```js
function filterMessage() {
  console.log(msg.rawContents);
  return MessageObject.Accept;
}
```

The above script produces this kind of debug output when running for Tiny Tiny RSS.
```
...
...
time="    34.360" type="debug" -> feed-downloader: Hooking message took 4 microseconds.
time="    34.361" type="debug" -> {"always_display_attachments":false,"attachments":[],"author":"Aleš Kapica","comments_count":0,"comments_link":"","content":"<p>\nNaposledy jsem psal o čuňačení v MediaWiki asi před půl rokem, kdy jsem chtěl upozornit na to, že jsem přepracoval svoji původní šablonu Images tak, aby bylo možné používat výřezy z obrázků a stránek generovaných z DjVu a PDF dokumentů. Blogpost nebyl nijak extra hodnocen, takže mě vcelku nepřekvapuje, jak se do hlavní vývojové větve MediaWiki dostávají čím dál větší prasečiny.\n</p>","feed_id":"5903","feed_title":"abclinuxu - blogy","flavor_image":"","flavor_stream":"","guid":"{\"ver\":2,\"uid\":\"52\",\"hash\":\"SHA1:5b49e4d8f612984889ba25e7834e80604c795ff8\"}","id":6958843,"is_updated":false,"labels":[],"lang":"","link":"http://www.abclinuxu.cz/blog/kenyho_stesky/2021/1/cunacime-v-mediawiki-responzivni-obsah-ii","marked":false,"note":null,"published":false,"score":0,"tags":[""],"title":"Čuňačíme v MediaWiki - responzivní obsah II.","unread":true,"updated":1610044674}
time="    34.361" type="debug" -> feed-downloader: Running filter script, it took 348 microseconds.
time="    34.361" type="debug" -> feed-downloader: Hooking message took 4 microseconds.
time="    34.361" type="debug" -> {"always_display_attachments":false,"attachments":[],"author":"kol-ouch","comments_count":0,"comments_link":"","content":"Ahoj, 1. 6. se blíží, tak začínám řešit co s bambilionem fotek na google photos. \n<p class=\"separator\"></p>\nZa sebe můžu říct, že gp mi vyhovují - ne snad úplně tím, že jsou zadarmo, ale hlavně způsobem práce s fotkami, možnostmi vyhledávání v nich podle obsahu, vykopírování textu z nich, provázaností s mapami, recenzemi, možnostmi sdílení, automatickým seskupováním a podobně.","feed_id":"5903","feed_title":"abclinuxu - blogy","flavor_image":"","flavor_stream":"","guid":"{\"ver\":2,\"uid\":\"52\",\"hash\":\"SHA1:1277107408b159882b95ca7151a0ec0160a3971a\"}","id":6939327,"is_updated":false,"labels":[],"lang":"","link":"http://www.abclinuxu.cz/blog/Co_to_je/2021/1/kam-s-fotkama","marked":false,"note":null,"published":false,"score":0,"tags":[""],"title":"Kam s fotkama?","unread":true,"updated":1609750800}
...
...
```

For RSS 2.0 message, the result might look like this.
```
...
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
```

Write details of available labels and assign the first label to the message.
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

Make sure that your receive only one message with particular URL across all your feeds (from same plugin) and all other messages with same URL are subsequently ignored.
```js
function filterMessage() {
  if (msg.isDuplicateWithAttribute(MessageObject.SameUrl | MessageObject.AllFeedsSameAccount)) {
    return MessageObject.Ignore;
  }
  else {
    return MessageObject.Accept;
  }
}
```

Remove "ads" from messages received from Inoreader. Method simply removes `div` which contains the advertisement.
```js
function filterMessage() {
  msg.contents = msg.contents.replace(/<div>\s*Ads[\S\s]+Remove<\/a>[\S\s]+adv\/www\/delivery[\S\s]+?<\/div>/im, '');

  return MessageObject.Accept;
}
```

## `Message filters` dialog
The dialog is accessible from menu `Messages -> Message filters` and is the central place for message filters management within RSS Guard. It allows you to:
* add or remove message filters,
* assign filter to whatever feeds (across all accounts) you want,
* rename filters and write their `JavaScript`-based scripts,
* reformat source code of script with `clang-format` tool (which is preinstalled on Windows version of RSS Guard),
* debug your script against sample `MessageObject` instance,
* debug your script agains list of real messages of some feed (available in RSS Guard 3.8.4+),
* execute message filters manually against feeds (available in RSS Guard 3.8.4+).

## Performance
Note that evaluations of JavaScript expressions are NOT that fast. They are much slower than native `C++` code, but well-optimized scripts usually take only several milliseconds to finish for each message.