Article Filtering
=================
Sometimes you need to automatically tweak an incoming article, mark it as starred, remove ads from its contents, or simply reject it. That is where the article filtering feature comes in.

## `Article filters` dialog
The dialog shown below offers a way to manage your article filters. You can assign a single filter to multiple feeds across all accounts. The dialog also allows you to test existing filters and see what changes have been made to articles during the filtering process.

The `Test` button tests the selected filter against existing messages without making permanent changes to them. `Process checked feeds` runs the filter against existing messages from checked feeds. In this mode, all modifications made by the filter are saved to the existing messages.

<img alt="alt-img" src="images/filters-dialog.png" width="600px">

## Writing an article filter
Article filters are small scripts that are executed automatically when articles or feeds are downloaded. Article filters are `JavaScript` snippets that must provide a function with this prototype:

```js
function filterMessage() { }
```

The function must return values that belong to the [`FilteringAction`](#filteringaction-enum) enumeration.

The supported built-in "standard library" adheres to [ECMA-262](https://ecma-international.org/publications-and-standards/standards/ecma-262).

Each article is accessible in your script via a global variable or property named `msg`. Some properties are writable, allowing you to change the contents of the article before it is written to the RSS Guard database. You can mark an article as important, change its description, change the author name, or even assign a [label](labels) to it.

```{note}
Some attributes such as `read`, `unread`, and `starred` states are synchronized back to your account's server. So, for example, you can mark some articles as starred and the change will be propagated back to the TT-RSS server if you use TT-RSS.
```

```{attention}
Special [placeholders](userdata.md#data-placeholder) can be used in article filters.
```

## Global variables

There are many global variables that provide information which can be used when filtering and modifying articles. Here is the list:

| Global variable | What it provides |
| :---            | :--- |
| `msg`           | Information about the actual article that is being filtered or modified. |
| `feed`          | All information about the feed to which the article belongs. |
| `acc`           | Account-related information. |
| `app`           | Application-wide information, like the RSS Guard version and so on. |
| `utils`         | A bunch of useful functions you might find handy in your filtering scripts. |
| `run`           | Information about the current article-filtering run. Contains the number of accepted articles so far, and so on. |
| `fs`            | Filesystem-related actions like process launching, file operations, and so on. |

## Reference Documentation

Here is the complete reference documentation of all functions and properties available for your filtering scripts.

### `msg`

#### Properties

| Name               | Type                      | Read-only | Synchronized | Description |
| :---               | :---                      | :---:     | :---:        | :--- |
| `assignedLabels`   | `Array<Label>`            | Yes       | Yes          | List of labels assigned to the article. |
| `categories`       | `Array<MessageCategory>`  | Yes       | No           | List of categories of the article, extracted from the feed. |
| `enclosures`       | `Array<MessageEnclosure>` | Yes       | No           | List of attachments of the article. |
| `id`               | `Number`                  | Yes       | No           | ID assigned to the message in the local RSS Guard database. |
| `customId`         | `String`                  | No        | No           | ID of the message as provided by the remote service or feed file. |
| `title`            | `String`                  | No        | No           | The message title. |
| `url`              | `String`                  | No        | No           | The message URL. |
| `author`           | `String`                  | No        | No           | Author of the message. |
| `contents`         | `String`                  | No        | No           | Contents of the message. |
| `rawContents`      | `String`                  | No        | No           | The raw contents of the message obtained from the remote service or feed. This is raw XML or JSON data. Note that this property is filled with data only when feeds or articles are fetched, not when processing existing articles with the article filter. |
| `score`            | `Number`                  | No        | No           | Arbitrary number in the range \<0.0, 100.0\>. You can use this number to sort messages in a custom fashion, as this attribute also has its own column in the article list. |
| `hasEnclosures`    | `Boolean`                 | Yes       | No           | Returns `true` if the article has at least one enclosure or attachment. Otherwise, it returns `false`. |
| `created`          | `Date`                    | No        | No           | Date and time of the message. |
| `createdIsMadeup`  | `Boolean`                 | Yes       | No           | Is `true` if the date and time of the message were not fetched from the feed and the current date and time were used instead. |
| `isRead`           | `Boolean`                 | No        | Yes          | Is the message read? |
| `isImportant`      | `Boolean`                 | No        | Yes          | Is the message important? |
| `isDeleted`        | `Boolean`                 | No        | No           | Is the message placed in the recycle bin? |

#### Functions

| Name(Parameters)                                                              | Return value | Description |
| :---                                                                          | :---         | :--- |
| `addEnclosure(String url, String mime_type)`                                  | `void`       | Adds a multimedia attachment to the article. |
| `removeEnclosure(int index)`                                                  | `Boolean`    | Removes one enclosure from the article according to the index, starting from zero. Returns `true` if the enclosure exists and was removed. |
| `removeAllEnclosures()`                                                       | `void`       | Removes all enclosures from the article. |
| `fetchFullContents(Boolean plain_text_only)`                                  | `Boolean`    | Fetches full article contents, plain text or HTML, for the article. [^1] |
| `isAlreadyInDatabase(DuplicityCheck criteria)`                                | `Boolean`    | Allows you to check if the same message is already stored in the RSS Guard database. |
| `isAlreadyInDatabaseWinkler(DuplicityCheck criteria, Number threshold = 0.1)` | `Boolean`    | Allows you to check if a similar message is already stored in the RSS Guard database by using the Jaro-Winkler edit-distance algorithm. |
| `assignLabel(String label_id)`                                                | `Boolean`    | Assigns a label to the message. The `String` value is the `customId` property of the `Label` type. See its API reference for relevant information. |
| `deassignLabel(String label_id)`                                              | `Boolean`    | Removes a label from the message. The `String` value is the `customId` property of the `Label` type. See its API reference for relevant information. |
| `deassignAllLabels()`                                                         | `void`       | Removes all labels from the message. |
| `exportCategoriesToLabels(Boolean assign_to_message)`                         | `void`       | Creates RSS Guard labels for all categories of this message. It can also optionally assign the message to those labels. |

[^1]: Note that fetching the full article content can have two important consequences. It could be slow, so it can slow down the whole feed-fetching process. Use this function sparingly. Also, the overall size of the database will grow if you store many full articles in it.

### `app`

#### Functions

| Name(Parameters)                              | Return value | Description |
| :---                                          | :---         | :--- |
| `log(String message)`                         | `void`       | Prints the message to the RSS Guard standard output and also to the "Script output" box if the "Article filters" dialog is displayed. |
| `showNotification(String title, String text)` | `void`       | Displays a toast desktop notification with the given title and text. |

### `run`

#### Properties

| Name                       | Type     | Read-only | Description |
| :---                       | :---     | :---:     | :--- |
| `numberOfAcceptedMessages` | `Number` | Yes       | Number of messages accepted so far in this feed-fetching run. |
| `indexOfCurrentFilter`     | `Number` | Yes       | Index of the currently executing filter, starting from 0. |
| `totalCountOfFilters`      | `Number` | Yes       | Total number of filters that will execute. |

### `acc`

#### Properties

| Name              | Type           | Read-only | Description |
| :---              | :---           | :---:     | :--- |
| `id`              | `Number`       | Yes       | Database ID of an account. |
| `title`           | `String`       | Yes       | Title of the account. |
| `availableLabels` | `Array<Label>` | Yes       | List of available labels that can be assigned to articles. |

#### Functions

| Name(Parameters)                                         | Return value | Description |
| :---                                                     | :---         | :--- |
| `findLabel(String label_title)`                          | `String`     | Finds a label with the given title. Returns the label ID. |
| `createLabel(String label_title, String hex_color_code)` | `String`     | Creates a label with the given title and color. Returns the label ID. |

### `feed`

#### Properties

| Name       | Type     | Read-only | Description |
| :---       | :---     | :---:     | :--- |
| `customId` | `String` | Yes       | Custom ID of the feed. |
| `title`    | `String` | Yes       | Title of the feed. |

### `utils`

#### Properties

| Name       | Type     | Read-only | Description |
| :---       | :---     | :---:     | :--- |
| `hostname` | `String` | Yes       | Name of your PC. |

#### Functions

| Name(Parameter)                                             | Return value  | Description |
| :---                                                        | :---          | :--- |
| `fromXmlToJson(String xml_string)`                          | `String`      | Converts an XML string into JSON. |
| `readFile(String filename)`                                 | `ArrayBuffer` | Reads a file into an array-buffer object. |
| `writeFile(String filename, ArrayBuffer data)`              | `void`        | Writes a file from an array-buffer object. |
| `readTextFile(String filename)`                             | `String`      | Reads a file into a UTF-8 string. |
| `writeTextFile(String filename, String text)`               | `void`        | Writes a file from a UTF-8 string. |
| `parseDateTime(String date_time)`                           | `Date`        | Converts a textual date-time representation into a proper `Date` object. |

### `fs`

#### Functions

| Name(Parameter)                                                                | Return value  | Description |
| :---                                                                           | :---          | :--- |
| `runExecutable(String exec, Array<String> params, String stdin_data)`          | `void`        | Launches an external executable with optional parameters and does not wait until the executable finishes. It also allows passing data to standard input. |
| `runExecutableGetOutput(String exec, Array<String> params, String stdin_data)` | `String`      | Launches an external executable with optional parameters, reads its standard output, and returns the output when the executable finishes. It also allows passing data to standard input. |

### Shared Data Types

#### `FilteringAction` enum

| Enumerant name | Integer value | Description |
| :---           | :---          | :--- |
| `Accept`       | 1             | Message is accepted and will be added or updated in the DB. |
| `Ignore`       | 2             | Message is ignored and will **NOT** be added or updated in the DB. An already existing message will not be purged from the DB. |
| `Purge`        | 4             | An existing message is purged from the DB completely. Behaves like `Ignore` when there is a new incoming message. |

#### `DuplicityCheck` enum

| Enumerant name        | Integer value | Description |
| :---                  | :---          | :--- |
| `SameTitle`           | 1             | Check if the message has the same title as another message. |
| `SameUrl`             | 2             | Check if the message has the same URL as another message. |
| `SameAuthor`          | 4             | Check if the message has the same author as another message. |
| `SameDateCreated`     | 8             | Check if the message has the same creation date as another message. |
| `AllFeedsSameAccount` | 16            | Perform the check across all feeds in your account, not just the current feed. |
| `SameCustomId`        | 32            | Check if a message with the same custom ID exists in the RSS Guard DB. |

#### `MessageEnclosure`

##### Properties

| Name       | Type     | Read-only | Description |
| :---       | :---     | :---:     | :--- |
| `url`      | `String` | Yes       | URL of the message enclosure. |
| `mimeType` | `String` | Yes       | MIME type of the message enclosure. |

#### `MessageCategory`

##### Properties

| Name    | Type     | Read-only | Description |
| :---    | :---     | :---:     | :--- |
| `title` | `String` | Yes       | Title of the message category. |

#### `Label`

##### Properties

| Name       | Type     | Read-only | Description |
| :---       | :---     | :---:     | :--- |
| `title`    | `String` | Yes       | Label name. |
| `customId` | `String` | Yes       | Service-specific ID of the label. The ID is used as a unique identifier for the label. It is useful if you want to assign or unassign the message label. |

```{attention}
The `msg` attributes are synchronized with the service even if you return `Purge` or `Ignore`. In other words, even if the filter ignores the article, you can still tweak its properties, and they will be synchronized back to your server.
```

## Examples

```js
/*
 * Accept whitelisted articles based on regular-expression filtering.
 */
function filterMessage() {
  // List of regular expressions.
  const whitelist = [
    /ubuntu.+desktop/i,
    /linux.+app/i,
    /\d.billion/i];

  // Accept article if it meets at least one of those regular expressions.
  if (whitelist.some(re => re.test(msg.title))) {
    return Msg.Accept;
  }

  return Msg.Ignore;
}
```

```js
/*
 * Mark all whitelisted messages as important.
 */
function filterMessage() {
  const whitelist = [
    /stock/i,
    /crypto/i,
    /market/i];

  if (whitelist.some(re => re.test(msg.title))) {
    msg.isImportant = true;
  }

  return Msg.Accept;
}
```

```js
/*
 * Ignore incoming articles which have very SIMILAR (not same) URL
 * as some article stored in database.
 *
 * Use very high similarity factor 0.05 which ensures that only highly
 * similar URLs are considered "same".
 *
 * This effectively filters out all incoming articles which are very similar
 * to the articles we already have.
 */
function filterMessage() {
  if (msg.isAlreadyInDatabaseWinkler(Msg.SameUrl, 0.05)) {
    return Msg.Ignore;
  }

  return Msg.Accept;
}
```

```js
/*
 * Check if similar article is present in database and if it is not
 * then fetch FULL article contents for it.
 */
function filterMessage() {
  if (!msg.isAlreadyInDatabaseWinkler(Msg.SameTitle, 0.05)) {
    msg.fetchFullContents(false);
    return Msg.Accept;
  } else {
    return Msg.Ignore;
  }
}
```

```js
/*
 * Ignore articles which are older than 7 days.
 *
 * Note that there already is built-in functionality for this date-based filtering,
 * but if you want to use an article filter for it, then this is the way.
 */
function filterMessage() {
  let now = new Date();
  let age = (now - msg.created) / (1000 * 60 * 60 * 24);

  if (age > 7) {
    return Msg.Ignore;
  }

  return Msg.Accept;
}
```

```js
/*
 * Assign label "AI" to all articles matching at least one whitelist condition.
 *
 * Make sure to have label "AI" created beforehand.
 */
function filterMessage() {
  if (/AI|robot|software|hardware/i.test(msg.title)) {
    let id = acc.findLabel("AI");

    if (id) {
      msg.assignLabel(id);
    }
  }

  return Msg.Accept;
}
```

```js
/*
 * Display a desktop notification when totally new breaking news arrives.
 */
function filterMessage() {
  if (/breaking/i.test(msg.title) && !msg.isAlreadyInDatabaseWinkler(Msg.SameTitle, 0.05)) {
    app.showNotification("Breaking News", msg.title);
    msg.isImportant = true;
  }

  return Msg.Accept;
}
```

```js
/*
 * Automatically create RSS Guard labels for all article categories defined for this article in a feed source.
 *
 * This supports RSS, ATOM and JSON category standards.
 */
function filterMessage() {
  msg.exportCategoriesToLabels(true);
  return Msg.Accept;
}
```

```js
/*
 * If the article link points to a YouTube video, then add it as an article attachment or enclosure.
 */
function filterMessage() {
  if (/youtube\.com/.test(msg.url)) {
    msg.addEnclosure(msg.url, "video/mp4");
  }

  return Msg.Accept;
}
```

```js
/*
 * Set the first article attachment as the main article URL and then remove all attachments.
 */
function filterMessage() {
  if (msg.enclosures.length > 0) {
    msg.url = msg.enclosures[0].url;
    msg.removeAllEnclosures();
  }

  return Msg.Accept;
}
```

```js
/*
 * Set the first article attachment as the main article URL and then remove it from the list of attachments.
 */
function filterMessage() {
  if (msg.enclosures.length > 0) {
    msg.url = msg.enclosures[0].url;
    msg.removeEnclosure(0);
  }

  return Msg.Accept;
}
```

```js
/*
 * Assign a score to articles based on keywords from an external file.
 *
 * Each line in `%data%/keywords.txt` should be:
 *   keyword,score
 *
 * Example:
 *   crypto,50
 *   blockchain,30
 *
 * Scores are additive but capped at 100.
 */
function filterMessage() {
  let keywords = [];

  try {
    let fileContent = utils.readTextFile('%data%/keywords.txt');
    keywords =
        fileContent.split(/\r?\n/)
            .map(line => {
              let parts = line.split(',');
              if (parts.length === 2) {
                return {term: parts[0].trim(), score: Number(parts[1].trim())};
              }
              return null;
            })
            .filter(k => k && !isNaN(k.score));
  } catch (e) {
    app.log('Keywords file missing -> default score 0.');
    msg.score = 0;
    return Msg.Accept;
  }

  let totalScore = 0;

  for (let k of keywords) {
    let re = new RegExp(k.term, 'i');
    if (re.test(msg.title) || re.test(msg.contents)) {
      totalScore += k.score;
    }
  }

  // Cap score at 100.
  if (totalScore > 100) {
    totalScore = 100;
  }

  msg.score = totalScore;

  // Optional: mark as important if high score.
  if (msg.score >= 70) {
    msg.isImportant = true;
  }

  return Msg.Accept;
}
```

```js
/*
 * Read a whitelist from an external text file and then use that whitelist to filter
 * the articles.
 */
function filterMessage() {
  let keywords = [];

  try {
    let fileContent = utils.readTextFile('%data%\\whitelist.txt');
    keywords = fileContent.split(/\r?\n/).filter(line => line.trim() !== '');
  } catch (e) {
    app.log('No whitelist file found, accepting all articles.');
  }

  for (let k of keywords) {
    let re = new RegExp(k, 'i');
    if (re.test(msg.title) || re.test(msg.contents)) {
      msg.isImportant = true;
      return Msg.Accept;
    }
  }

  return Msg.Ignore;
}
```

```js
/*
 * Converts HTML article contents to pure plain text, removing all HTML formatting. It also
 * converts all hyperlinks to plain text.
 */
function filterMessage() {
  let text = msg.contents;

  // Convert links: <a href="URL">Text</a> -> Text (URL).
  text = text.replace(
      /<a[^>]+href="([^"]+)"[^>]*>(.*?)<\/a>/gi, (m, url, label) => {
        return label + ' (' + url + ')';
      });

  // Remove all other HTML tags.
  text = text.replace(/<[^>]*>/g, '');

  // Extended HTML entities dictionary.
  const entities = {
    '&amp;': '&',
    '&lt;': '<',
    '&gt;': '>',
    '&quot;': '"',
    '&#39;': '\'',
    '&nbsp;': ' ',
    '&copy;': '(c)',
    '&reg;': '(R)',
    '&euro;': 'EUR',
    '&pound;': 'GBP',
    '&yen;': 'JPY',
    '&cent;': 'cent',
    '&sect;': 'section',
    '&para;': 'paragraph',
    '&bull;': '*',
    '&ndash;': '-',
    '&mdash;': '--',
    '&lsquo;': '\'',
    '&rsquo;': '\'',
    '&ldquo;': '"',
    '&rdquo;': '"',
    '&hellip;': '...',
    '&trade;': 'TM',
    '&deg;': 'deg',
    '&plusmn;': '+/-',
    '&times;': 'x',
    '&divide;': '/'
  };

  // Replace entities with plain text.
  text = text.replace(/&[a-z#0-9]+;/gi, (entity) => entities[entity] || entity);

  // Normalize whitespace.
  text = text.replace(/\s+/g, ' ').trim();

  msg.contents = text;

  return Msg.Accept;
}
```

```js
/*
 * Convert article contents from HTML to plain text with Pandoc.
 *
 * This uses the Pandoc binary placed directly in the user-data folder.
 * Article contents are passed to Pandoc via standard input.
 */
function filterMessage() {
  let res = fs.runExecutableGetOutput(
      '%data%\\pandoc.exe',
      ['-f', 'html', '-t', 'plain'],
      msg.contents);

  msg.contents = res;

  return Msg.Accept;
}
```

```js
/*
 * Embed all images directly into the article, so that the article fully works while offline.
 *
 * This calls an external Python script (see below) that examines the article contents, downloads all images
 * and stores their data directly into the article.
 *
 * Note that this script is fairly slow, so you should pair it with the "acc.isAlreadyInDatabase()" function
 * and use it sparingly, as the RSS Guard database will grow immensely.
 */
function filterMessage() {
  let res = fs.runExecutableGetOutput(
      'python3.exe', ['%data%\\img2base.py'], msg.contents);

  msg.contents = res;

  return Msg.Accept;
}
```

```python
# img2base.py
import sys, io, base64, requests, mimetypes
from bs4 import BeautifulSoup

sys.stdin = io.TextIOWrapper(sys.stdin.buffer, encoding="utf-8")
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8")

soup = BeautifulSoup(sys.stdin.read(), "html.parser")

for img in soup.find_all("img"):
    src = img.get("src")
    if src and src.startswith(("http://", "https://")):
        try:
            r = requests.get(src, timeout=10)
            r.raise_for_status()
            mime, _ = mimetypes.guess_type(src)
            if not mime:
                mime = "application/octet-stream"
            b64 = base64.b64encode(r.content).decode("utf-8")
            img["src"] = f"data:{mime};base64,{b64}"
        except:
            continue

sys.stdout.write(str(soup))
```

```js
/*
 * Translate the article title using an external Python script (see below) and also print it to the "Script output" dialog.
 *
 * Python sometimes adds extra newlines at the end of its standard output, so trim the translated
 * title to make sure it is all nice and clean.
 *
 * Note that the translation script calls a remote service to do the translation, so it is NOT fast. Use it sparingly.
 */
function filterMessage() {
  msg.title = fs.runExecutableGetOutput("python3.exe", ["trans-stdin.py", "en", "cs"], msg.title);

  msg.title = msg.title.trim();
  app.log(msg.title);

  return Msg.Accept;
}
```

```python
import sys
import asyncio
from googletrans import Translator

async def translate_string(to_translate, lang_src, lang_dest):
    async with Translator() as translator:
        translated_text = await translator.translate(to_translate, src=lang_src, dest=lang_dest)
        print(translated_text.text)

lang_from = sys.argv[1]
lang_to = sys.argv[2]

sys.stdin.reconfigure(encoding="utf-8")
sys.stdout.reconfigure(encoding="utf-8")

data = sys.stdin.read()

asyncio.run(translate_string(data, lang_from, lang_to))
```
