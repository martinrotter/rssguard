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
When `-t` and `-b` are combined, the output remains plain text and images are not downloaded.

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

Configured headers apply to the supplied URL's origin (scheme, hostname, and port). Only `User-Agent`, `Accept`, and `Accept-Language` are forwarded to other origins, including image hosts and redirect destinations. This prevents configured cookies, authorization, and other private headers from being sent to unrelated sites. Cookies set by websites are shared between page and image downloads using normal cookie domain, path, and security rules. Updated cookies take precedence over the same cookie supplied in `headers`.

### `html`
Use `html` when you already have the page contents. When `html` is non-empty, the extractor does **not** download the URL. It cleans up the supplied HTML instead.

This is useful when you already have article HTML, for example from an RSS item, a browser cache, or an [article filter](filters).

### `proxy`
Use `proxy` when page downloads or image downloads should go through a proxy.

Supported proxy types:
* `http`
* `socks5`

`address` must contain host and port, for example `127.0.0.1:8080`.
Usernames and passwords may contain special characters; pass them as JSON strings without URL-encoding them.

## Redirects and Images
Relative links and images are resolved against the final page URL after redirects. With supplied `html`, they use the URL argument instead.

With `-b`, each image URL is downloaded at most once per invocation. Successfully embedded images have their `srcset` alternatives and associated `<picture>` sources removed so a browser uses the embedded image. Failed or oversized images keep their original URLs and responsive alternatives. Image embedding is best effort; the result may still require a network connection for images that could not be embedded or other resources.

## Limits
Page and image requests share a 25-second network deadline. A timeout fails the invocation without writing partial article output. The limits below also apply:

| Resource | Limit |
| :--- | :--- |
| JSON input | 16 MiB |
| Supplied or downloaded HTML | 10 MiB |
| Each downloaded image | 5 MiB |
| Embedded image data, including repeated occurrences | 20 MiB |
| Extracted output | 32 MiB |

Oversized input, pages, or output fail with an error. Images exceeding the individual or total embedding limit remain remote.

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

If extraction fails, an error is written to standard error and the exit code is nonzero. Common causes are an invalid URL, malformed JSON, a network failure, an invalid proxy, a size limit, an empty extraction result, or a page that cannot be parsed. The URL must be an absolute HTTP or HTTPS URL, even when supplying HTML. Empty or whitespace-only stdin uses the defaults.

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

For most ordinary filters, prefer `msg.fetchFullContents(...)` because it uses the bundled extractor with RSS Guard's user agent, language, and applicable proxy settings. Call the extractor manually when you need custom headers, direct control over arguments, or want to pass a custom `html` string.

## Building and Testing
Go 1.26.0 or newer is required. CI uses Go 1.27.3. CMake builds the complete Go package and tracks source, module manifest, vendor, and compiler changes. Windows builds install `rssguard-article-extractor.exe`.

Run these commands in `resources/scripts/standalone/article-extractor`:

```sh
go test -count=1 ./...
go vet ./...
go mod tidy -diff
go run golang.org/x/vuln/cmd/govulncheck@v1.9.0 ./...
python build_test.py
```

The CMake build-rule tests need CMake and Ninja. The Go tests are also registered with CTest when `ENABLE_TESTING` is enabled. Run vulnerability checks with a scanner built using the same or a newer Go compiler than the code being checked.

Dependency maintenance is separate from building: use `go get` for deliberate updates, run `go mod tidy`, and regenerate an existing vendor directory with `go mod vendor`. CMake no longer runs `go mod tidy` or changes the module manifests while building. Source release archives include vendored dependencies.
