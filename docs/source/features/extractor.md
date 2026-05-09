Article Extractor
=================
RSS Guard ships a standalone helper named `rssguard-article-extractor`. It is used internally by the `Fetch full articles` feature and by the article-filtering function `msg.fetchFullContents(...)`, but advanced users can also call it directly from scripts or other tools.

The extractor takes a web page, finds the main readable article content, and writes that cleaned-up article to standard output. It can either download the page from a URL or process HTML that you already have.

## Where to Find It
The extractor is installed next to the main RSS Guard executable.

Typical names:
* Windows: `rssguard-article-extractor.exe`
* Linux/macOS: `rssguard-article-extractor`

## Basic Usage
```text
rssguard-article-extractor [options] <url>
```

The URL is required. If you do not pass HTML through standard input, the extractor downloads this URL. If you do pass HTML through standard input, the URL is still used as the article's base address.

## Options
| Option | Description |
| :---   | :--- |
| `-t`   | Output plain text instead of HTML. |
| `-b`   | Embed remote images as `data:` URLs in the HTML output. |

Without `-t`, the extractor writes extracted HTML.

## Optional JSON Input
You can pass extra settings as JSON through standard input. If you do not pass JSON, the extractor uses its defaults.

```json
{
  "headers": {
    "User-Agent": "Custom user agent",
    "Accept-Language": "en-US"
  },
  "html": "<!doctype html><html>...</html>",
  "proxy": {
    "type": "http",
    "address": "127.0.0.1:8080",
    "username": "optional-user",
    "password": "optional-password"
  }
}
```

All fields are optional.

### `headers`
Use `headers` when a site needs a specific user agent, language, cookie, authorization header, or another HTTP header.

If `User-Agent` is not provided, RSS Guard's extractor uses its built-in default browser-like user agent.

### `html`
Use `html` when you already have the page contents. When `html` is non-empty, the extractor does **not** download the URL. It cleans up the supplied HTML instead.

This is useful when you already have article HTML, for example from an RSS item, a browser cache, or an [article filter](filters).

### `proxy`
Use `proxy` when page downloads or image downloads should go through a proxy.

Supported proxy types:
* `http`
* `socks5`

`address` must contain host and port, for example `127.0.0.1:8080`.

## Examples
### Extract HTML From a URL
```bash
rssguard-article-extractor "https://example.com/article"
```

### Extract Plain Text
```bash
rssguard-article-extractor -t "https://example.com/article"
```

### Extract HTML and Embed Images
```bash
rssguard-article-extractor -b "https://example.com/article"
```

### Pass Custom Headers
```bash
printf '{"headers":{"User-Agent":"RSS Guard script","Accept-Language":"en-US"}}' \
  | rssguard-article-extractor "https://example.com/article"
```

PowerShell:

```powershell
'{"headers":{"User-Agent":"RSS Guard script","Accept-Language":"en-US"}}' |
  rssguard-article-extractor "https://example.com/article"
```

### Use Existing HTML Instead of Downloading the URL
```bash
printf '{"html":"<!doctype html><html><body><article><h1>Hello</h1><p>Body.</p></article></body></html>"}' \
  | rssguard-article-extractor "https://example.com/article"
```

PowerShell:

```powershell
'{"html":"<!doctype html><html><body><article><h1>Hello</h1><p>Body.</p></article></body></html>"}' |
  rssguard-article-extractor "https://example.com/article"
```

### Use an HTTP Proxy
```bash
printf '{"proxy":{"type":"http","address":"127.0.0.1:8080"}}' \
  | rssguard-article-extractor "https://example.com/article"
```

### Use a SOCKS5 Proxy With Authentication
```bash
printf '{"proxy":{"type":"socks5","address":"127.0.0.1:1080","username":"user","password":"pass"}}' \
  | rssguard-article-extractor "https://example.com/article"
```

## Errors
If extraction succeeds, the cleaned article is written to standard output.

If extraction fails, an error is written to standard error. Common causes are an invalid URL, a network failure, an invalid proxy, or a page that cannot be parsed.

When `-b` is used, individual image download failures are not fatal. If an image cannot be downloaded, its original `src` value is left unchanged.

## Calling From Article Filters
Article filters can call the extractor directly with `fs.runExecutableGetOutput(...)`. This can be useful when you want readability cleanup for HTML that is already present in `msg.contents`.

```js
function filterMessage() {
  if (!msg.url || !msg.contents) {
    return Msg.Accept;
  }

  const extractor =
    "C:\\Path\\To\\rssguard-article-extractor.exe";

  const config = JSON.stringify({
    html: msg.contents
  });

  const extracted = fs.runExecutableGetOutput(
    extractor,
    [msg.url],
    config
  );

  if (extracted && extracted.trim()) {
    msg.contents = extracted.trim();
  }

  return Msg.Accept;
}
```

For plain-text output, pass `-t` before the URL:

```js
const extracted = fs.runExecutableGetOutput(
  extractor,
  ["-t", msg.url],
  config
);
```

For most ordinary filters, prefer `msg.fetchFullContents(...)` because it uses RSS Guard's configured extractor path and feed settings automatically. Call the extractor manually when you need direct control over arguments or want to pass a custom `html` string.
