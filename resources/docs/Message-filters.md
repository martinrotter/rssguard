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

This function must be fast and must return values which belong to enumeration `FilteringAction` from this [file](https://github.com/martinrotter/rssguard/blob/master/src/librssguard/core/message.h). You can you either direct numerical value of each enumerant, for example `2` or you can use self-descriptive enumerant name, for example `MessageObject.Ignore`. Named enumerants are supported in RSS Guard 3.8.1+. RSS Guard 3.7.1+ also offers names `MSG_ACCEPT` and `MSG_IGNORE` as aliases for `MessageObject.Accept` and `MessageObject.Ignore`.

Each message is accessible in your script via global variable named `msg` of type `MessageObject`, see this [file](https://github.com/martinrotter/rssguard/blob/master/src/librssguard/core/message.h) for the declaration. Some properties are writable, allowing you to change contents of the message before it is written to DB. You can mark message important, parse its description or perhaps change author name or even assign some label to it!!!

RSS Guard 3.8.0+ offers also list of labels assigned to each message. You can therefore do actions in your filtering script based on which labels are assigned to the message. The property is called `assignedLabels` and is array of `Label` objects. Each `Label` in the array offers these properties: `title` (title of the label), `color` (color of the label) and `customId` (account-specific ID of the label). If you change assigned labels to the message, then the change will be eventually synchronized back to server if respective plugin supports it.

Passed message also offers special function
```js
MessageObject.isDuplicateWithAttribute(DuplicationAttributeCheck)
```

which allows you to perform runtime check for existence of the message in RSS Guard's database. The parameter is integer value from enumeration `DuplicationAttributeCheck` from this [file](https://github.com/martinrotter/rssguard/blob/master/src/librssguard/core/message.h) and specifies how exactly you want to determine if given message is "duplicate". Again, you can use direct integer value or enumerant name.

For example if you want to check if there is already another message with same author in database, then you call `msg.isDuplicateWithAttribute(MessageObject.SameAuthor)`. Enumeration even supports "flags" approach, thus you can combine multiple checks via bitwise `OR` operation in single call, for example like this: `msg.isDuplicateWithAttribute(MessageObject.SameAuthor | MessageObject.SameUrl)`.

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
| `Date created` | Date/time of the message. |
| `Boolean isRead` | Is message read? |
| `Boolean isImportant` | Is message important? |
| `Boolean isDeleted` | Is message placed in recycle bin? Available in RSS Guard 3.8.4+. |
| `Boolean isDuplicateWithAttribute(DuplicationAttributeCheck)` | Allows you to test if this particular message is already stored in RSS Guard's DB. |
| `Boolean assignLabel(String)` | Assigns label to this message. The passed `String` value is the `customId` property of `Label` type. See its API reference for relevant info. Available in RSS Guard 3.8.1+. |
| `Boolean deassignLabel(String)` | Removes label from this message. The passed `String` value is the `customId` property of `Label` type. See its API reference for relevant info. Available in RSS Guard 3.8.1+. |
| `Boolean alreadyStoredInDb` | `READ-ONLY` Returns true if this message is already stored in DB. This function is the way to check if the filter is being run automatically for newly downloaded messages or manually for already existing messages. Available in RSS Guard 3.8.4+. |

### `Label` class
| Property/method | Description |
|---|---|
| `String title` | `READ-ONLY` Label title. |
| `String customId` | `READ-ONLY` Service-specific ID of this label. This ID is used as unique identifier for the label and is particularly useful if you want to (de)assign label to/from message. |
| `color color` | `READ-ONLY` Label color. Note that type `color` has its documentation [here](https://doc.qt.io/qt-5/qml-color.html). |

### `FilteringAction` enum
| Enumerant name | Integer value | Description |
|---|---|---|
| Accept | 1 | Message is accepted and will be added to DB or updated in DB. |
| Ignore | 2 | Message is ignored and will be **NOT** added to DB or updated in DB, but is not purged from DB if already exists. |
| Purge | 4 | Existing message is purged from the DB completely. |

Note that `MessageObject` attributes which can be synchronized back to service are synchronized even if you return `Purge` or `Ignore`. In other words: even if you filter ignores the message you can still tweak its properties which will get synchronized back to your server.

### `DuplicationAttributeCheck` enum
| Enumerant name | Integer value | Description |
|---|---|---|
| SameTitle | 1 | Check if message has same title as some another messages. |
| SameUrl | 2 | Check if message has same URL as some another messages. |
| SameAuthor | 4 | Check if message has same author as some another messages. |
| SameDateCreated | 8 | Check if message has same date of creation as some another messages. |
| AllFeedsSameAccount | 16 | Perform the check across all feeds from your account, not just "current" feed. |

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

Write details of available labels and assign the first label to the message.
```js
function filterMessage() {
  console.log('Number of assigned labels ' + msg.assignedLabels.length);
  console.log('Number of available labels ' + msg.availableLabels.length);

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

Make sure that your receive only one message with particular URL and all other messages with same URL are subsequently ignored.
```js
function filterMessage() {
  if (msg.isDuplicateWithAttribute(MessageObject.SameUrl)) {
    return MessageObject.Ignore;
  }
  else {
    return MessageObject.Accept;
  }
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