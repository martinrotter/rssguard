Article Extractor
=================
RSS Guard ships a standalone helper named `rssguard-article-extractor`. It is used internally by the `Fetch full articles` feature and by the article-filtering function `msg.fetchFullContents(...)`, but advanced users can also call it directly from scripts or other tools.

The extractor downloads or receives an HTML page, runs readability extraction on it, and writes the extracted article to standard output.

## Location
In an installed RSS Guard build, the binary is placed next to the main RSS Guard executable.

Typical names:
* Windows: `rssguard-article-extractor.exe`
* Linux/macOS: `rssguard-article-extractor`

When building from source, CMake places it under the RSS Guard build output directory, usually near:

```text
<build-directory>/src/rssguard/rssguard-article-extractor
```

## Synopsis
```text
rssguard-article-extractor [options] <url>
```

The `<url>` argument is always required. It is used as the page URL when the extractor downloads the page itself, and as the base URL when the HTML is supplied through standard input.

## Options
| Option | Description |
| :---   | :--- |
| `-t`   | Output plain text instead of HTML. |
| `-b`   | Embed remote images as `data:` URLs in the HTML output. |

Without `-t`, the extractor writes extracted HTML.

## Standard Input Configuration
The extractor optionally reads a JSON object from standard input. If no JSON is supplied, default settings are used.

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
`headers` is an object whose keys and values are sent with page downloads and image downloads.

If `User-Agent` is not provided, RSS Guard's extractor uses its built-in default browser-like user agent.

### `html`
`html` is an optional HTML string. When it is non-empty, the extractor does **not** download `<url>`. Instead, it runs readability extraction on this supplied HTML and uses `<url>` only as the base URL.

This is useful when you already have article HTML, for example from an RSS item, a browser cache, or an [article filter](filters).

### `proxy`
`proxy` configures network access for page downloads and remote image downloads.

Supported proxy types:
* `http`
* `socks5`

`address` must contain host and port, for example `127.0.0.1:8080`.

## Exit Behavior
On success, the extractor writes the extracted article to standard output and exits with code `0`.

On error, it writes a diagnostic message to standard error and exits with a non-zero code.

Common error cases include:
* invalid URL
* network request failure
* non-2xx HTTP response when downloading the page
* invalid proxy configuration
* readability parsing failure

Image download failures during `-b` are ignored for individual images. If an image cannot be downloaded, its original `src` value is left unchanged.

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
