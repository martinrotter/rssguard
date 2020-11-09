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

Message filter consists of arbitrary JavaScript code which must provide function with prototype `function filterMessage() { }`. This function must be fast and must return integer values which belong to enumeration [`FilteringAction`](https://github.com/martinrotter/rssguard/blob/master/src/librssguard/core/message.h#L83). For example, your function must return `2` to block the message which is subsequently NOT saved into database. For easier usage, RSS Guard 3.7.1+ offers named variables for this, which are called `MSG_ACCEPT` and `MSG_IGNORE`.

Each message is accessible in your script via global variable named `msg` of type [`MessageObject`](https://github.com/martinrotter/rssguard/blob/master/src/librssguard/core/message.h#L118). Some properties are writable, thus allowing you to change contents of the message before it is written to DB. You can mark message important, parse its description or perhaps change author name!!!

RSS Guard 3.8.0+ offers also read-only list of labels assigned to each message. You can therefore do actions in your filtering script based on which labels are assigned to the message. The property is called `assignedLabels` and is array of `Label` objects. Each `Label` in the array offers these properties: `title` (title of the label), `color` (color of the label) and `customId` (account-specific ID of the label).

Passed message also offers special function
```js
MessageObject.isDuplicateWithAttribute(DuplicationAttributeCheck)
```
which allows you to perform runtime check for existence of the message in RSS Guard's database. The parameter is integer value from enumeration [`DuplicationAttributeCheck`](https://github.com/martinrotter/rssguard/blob/master/src/librssguard/core/message.h#L91) and specifies how exactly you want to determine if given message is "duplicate".

For example if you want to check if there is already another message with same author in database, then you call `msg.isDuplicateWithAttribute(4)`. Enumeration even supports "flags" approach, thus you can combine multiple checks via bitwise `OR` operation in single call, for example like this: `msg.isDuplicateWithAttribute(4 | 16)`.

## Examples
Accept only messages from "Bob" while also marking them important.
```js
function filterMessage() {
  if (msg.author == "Bob") {
    msg.isImportant = true;
    return MSG_ACCEPT;
  }
  else {
    return MSG_IGNORE;
  }
}
```

Replace all dogs with cats!
```js
function filterMessage() {
  msg.title = msg.title.replace("dogs", "cats");
  return MSG_ACCEPT;
}
```
## `Message filters` dialog
The dialog is accessible from menu `Messages -> Message filters` and is the central place for message filters management within RSS Guard. It allows you to:
* add or remove message filters,
* assign filter to whatever feeds (across all accounts) you want,
* rename filters and write their `JavaScript`-based scripts,
* reformat source code of script with `clang-format` tool (which is preinstalled on Windows version of RSS Guard),
* debug your script against sample `MessageObject` instance.

## Performance
Note that evaluations of JavaScript expressions are NOT that fast. They are much slower than native `C++` code, but well-optimized scripts usually take only several milliseconds to finish for each message.