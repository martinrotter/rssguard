package main

import (
	"bytes"
	"context"
	"encoding/base64"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"io"
	"mime"
	"net"
	"net/http"
	"net/http/cookiejar"
	"net/url"
	"os"
	"strings"
	"time"

	"codeberg.org/readeck/go-readability/v2"
	"github.com/gocolly/colly/v2"
	"golang.org/x/net/html"
	"golang.org/x/net/idna"
	"golang.org/x/net/proxy"
	"golang.org/x/net/publicsuffix"
)

const (
	DefaultUserAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0 Safari/537.36"
	operationTimeout = 25 * time.Second // Finish network work before RSS Guard's 30-second process wait.
	maxConfigBytes   = 16 << 20
	maxHTMLBytes     = 10 << 20
	maxImageBytes    = 5 << 20
	maxEmbeddedBytes = 20 << 20
	maxOutputBytes   = 32 << 20
)

type ProxyConfig struct {
	Type     string `json:"type"`
	Address  string `json:"address"`
	Username string `json:"username"`
	Password string `json:"password"`
}

type InputConfig struct {
	Headers map[string]string `json:"headers"`
	HTML    string            `json:"html"`
	Proxy   *ProxyConfig      `json:"proxy"`
}

func readLimited(r io.Reader, limit int64) ([]byte, error) {
	data, err := io.ReadAll(io.LimitReader(r, limit+1))
	if err != nil {
		return nil, err
	}
	if int64(len(data)) > limit {
		return nil, fmt.Errorf("size exceeds %d bytes", limit)
	}
	return data, nil
}

func readConfig(input io.Reader) (InputConfig, error) {
	var cfg InputConfig
	data, err := readLimited(input, maxConfigBytes)
	if err != nil {
		return cfg, fmt.Errorf("reading configuration: %w", err)
	}
	if len(bytes.TrimSpace(data)) != 0 {
		if err := json.Unmarshal(data, &cfg); err != nil {
			return cfg, fmt.Errorf("invalid JSON configuration: %w", err)
		}
	}
	if len(cfg.HTML) > maxHTMLBytes {
		return cfg, fmt.Errorf("supplied HTML exceeds %d bytes", maxHTMLBytes)
	}
	return cfg, nil
}

func sameOrigin(a, b *url.URL) bool {
	host := func(u *url.URL) string {
		ascii, err := idna.Lookup.ToASCII(u.Hostname())
		if err == nil {
			return ascii
		}
		return u.Hostname() // IPv6 literals are compared directly.
	}
	port := func(u *url.URL) string {
		if u.Port() != "" {
			return u.Port()
		}
		if strings.EqualFold(u.Scheme, "https") {
			return "443"
		}
		return "80"
	}
	return strings.EqualFold(a.Scheme, b.Scheme) && strings.EqualFold(host(a), host(b)) && port(a) == port(b)
}

func publicHeader(name string) bool {
	switch http.CanonicalHeaderKey(name) {
	case "User-Agent", "Accept", "Accept-Language":
		return true
	}
	return false
}

func applyHeaders(req *http.Request, headers map[string]string, origin *url.URL, jar http.CookieJar) {
	req.Header.Set("User-Agent", DefaultUserAgent)
	for name, value := range headers {
		if sameOrigin(req.URL, origin) || publicHeader(name) {
			req.Header.Set(name, value)
		}
	}
	// Cookies updated by the page take precedence over the original configured value.
	stored := make(map[string]bool)
	for _, cookie := range jar.Cookies(req.URL) {
		stored[cookie.Name] = true
	}
	configured := req.Cookies()
	req.Header.Del("Cookie")
	for _, cookie := range configured {
		if !stored[cookie.Name] {
			req.AddCookie(cookie)
		}
	}
}

func setupProxy(cfg *ProxyConfig, transport *http.Transport) error {
	if cfg == nil {
		return nil
	}
	host, port, err := net.SplitHostPort(cfg.Address)
	if err != nil || host == "" || port == "" {
		return errors.New("invalid proxy address: expected host:port")
	}
	switch strings.ToLower(cfg.Type) {
	case "http":
		u := &url.URL{Scheme: "http", Host: cfg.Address}
		if cfg.Username != "" {
			u.User = url.UserPassword(cfg.Username, cfg.Password)
		}
		transport.Proxy = http.ProxyURL(u)
	case "socks5":
		var auth *proxy.Auth
		if cfg.Username != "" {
			auth = &proxy.Auth{User: cfg.Username, Password: cfg.Password}
		}
		dialer, err := proxy.SOCKS5("tcp", cfg.Address, auth, &net.Dialer{Timeout: operationTimeout})
		if err != nil {
			return fmt.Errorf("setting up SOCKS5 proxy: %w", err)
		}
		transport.Proxy = nil
		transport.DialContext = dialer.(proxy.ContextDialer).DialContext
	default:
		return errors.New("unsupported proxy type: expected http or socks5")
	}
	return nil
}

func newClient(cfg InputConfig, origin *url.URL) (*http.Client, error) {
	transport := http.DefaultTransport.(*http.Transport).Clone()
	if err := setupProxy(cfg.Proxy, transport); err != nil {
		return nil, err
	}
	jar, err := cookiejar.New(&cookiejar.Options{PublicSuffixList: publicsuffix.List})
	if err != nil {
		return nil, err
	}
	return &http.Client{
		Transport: transport,
		Jar:       jar,
		CheckRedirect: func(req *http.Request, via []*http.Request) error {
			if len(via) >= 10 {
				return errors.New("stopped after 10 redirects")
			}
			if !sameOrigin(req.URL, origin) {
				for name := range cfg.Headers {
					if !publicHeader(name) {
						req.Header.Del(name)
					}
				}
				req.Header.Del("Authorization")
				req.Header.Del("Cookie") // The cookie jar adds only cookies valid for the destination.
			}
			return nil
		},
	}, nil
}

func fetchPage(ctx context.Context, pageURL *url.URL, cfg InputConfig, client *http.Client) ([]byte, *url.URL, error) {
	collector := colly.NewCollector(colly.MaxBodySize(maxHTMLBytes + 1))
	collector.Context = ctx
	collector.SetClient(client)
	collector.OnRequest(func(r *colly.Request) {
		req := &http.Request{URL: r.URL, Header: *r.Headers}
		applyHeaders(req, cfg.Headers, pageURL, client.Jar)
	})
	var pageHTML []byte
	finalURL := pageURL
	collector.OnResponse(func(r *colly.Response) {
		pageHTML = r.Body
		finalURL = r.Request.URL
	})
	if err := collector.Visit(pageURL.String()); err != nil {
		return nil, nil, fmt.Errorf("fetching page: %w", err)
	}
	if len(pageHTML) > maxHTMLBytes {
		return nil, nil, fmt.Errorf("downloaded HTML exceeds %d bytes", maxHTMLBytes)
	}
	return pageHTML, finalURL, nil
}

type limitedWriter struct {
	io.Writer
	remaining int
}

func (w *limitedWriter) Write(p []byte) (int, error) {
	if len(p) > w.remaining {
		return 0, errors.New("extracted output exceeds size limit")
	}
	n, err := w.Writer.Write(p)
	w.remaining -= n
	return n, err
}

func renderArticle(article readability.Article, plainText bool) (string, error) {
	var content bytes.Buffer
	w := &limitedWriter{Writer: &content, remaining: maxOutputBytes}
	var err error
	if plainText {
		err = article.RenderText(w)
	} else {
		err = article.RenderHTML(w)
	}
	if err != nil {
		return "", fmt.Errorf("rendering article: %w", err)
	}
	if strings.TrimSpace(content.String()) == "" {
		return "", errors.New("extracted article is empty")
	}
	return content.String(), nil
}

func downloadImage(ctx context.Context, src string, client *http.Client, cfg InputConfig, origin *url.URL) string {
	req, err := http.NewRequestWithContext(ctx, "GET", src, nil)
	if err != nil {
		return ""
	}
	applyHeaders(req, cfg.Headers, origin, client.Jar)
	resp, err := client.Do(req)
	if err != nil {
		return ""
	}
	defer resp.Body.Close()
	if resp.StatusCode < 200 || resp.StatusCode >= 300 || resp.ContentLength > maxImageBytes {
		return ""
	}
	data, err := readLimited(resp.Body, maxImageBytes)
	if err != nil {
		return ""
	}
	mimeType, _, _ := mime.ParseMediaType(resp.Header.Get("Content-Type"))
	if !strings.HasPrefix(mimeType, "image/") {
		mimeType = http.DetectContentType(data)
	}
	if !strings.HasPrefix(mimeType, "image/") {
		return ""
	}
	return "data:" + mimeType + ";base64," + base64.StdEncoding.EncodeToString(data)
}

func embedImages(ctx context.Context, content string, client *http.Client, cfg InputConfig, origin *url.URL) (string, error) {
	doc, err := html.Parse(strings.NewReader(content))
	if err != nil {
		return "", err
	}
	cache := make(map[string]string)
	embeddedBytes := 0
	var traverse func(*html.Node)
	traverse = func(n *html.Node) {
		if ctx.Err() != nil {
			return
		}
		if n.Type == html.ElementNode && n.Data == "img" {
			for i, attr := range n.Attr {
				if attr.Key != "src" || !(strings.HasPrefix(attr.Val, "http://") || strings.HasPrefix(attr.Val, "https://")) {
					continue
				}
				dataURI, found := cache[attr.Val]
				if !found {
					dataURI = downloadImage(ctx, attr.Val, client, cfg, origin)
					if embeddedBytes+len(dataURI) > maxEmbeddedBytes {
						dataURI = ""
					}
					cache[attr.Val] = dataURI
				}
				if dataURI == "" || embeddedBytes+len(dataURI) > maxEmbeddedBytes {
					continue
				}
				embeddedBytes += len(dataURI)
				n.Attr[i].Val = dataURI
				// Responsive alternatives would otherwise override the embedded src in a browser.
				attrs := n.Attr[:0]
				for _, a := range n.Attr {
					if a.Key != "srcset" {
						attrs = append(attrs, a)
					}
				}
				n.Attr = attrs
				if n.Parent != nil && n.Parent.Data == "picture" {
					for c := n.Parent.FirstChild; c != nil; {
						next := c.NextSibling
						if c.Type == html.ElementNode && c.Data == "source" {
							n.Parent.RemoveChild(c)
						}
						c = next
					}
				}
				break
			}
		}
		for c := n.FirstChild; c != nil; c = c.NextSibling {
			traverse(c)
		}
	}
	traverse(doc)
	if err := ctx.Err(); err != nil {
		return "", err
	}
	var output strings.Builder
	if err := html.Render(&limitedWriter{Writer: &output, remaining: maxOutputBytes}, doc); err != nil {
		return "", err
	}
	return output.String(), nil
}

func run(ctx context.Context, args []string, input io.Reader, output, stderr io.Writer) error {
	flags := flag.NewFlagSet("rssguard-article-extractor", flag.ContinueOnError)
	flags.SetOutput(stderr)
	plainText := flags.Bool("t", false, "Output plain text instead of HTML")
	embedImgs := flags.Bool("b", false, "Embed images as base64 (HTML output only)")
	if err := flags.Parse(args); err != nil {
		return err
	}
	if flags.NArg() != 1 {
		return errors.New("usage: rssguard-article-extractor [options] <url>")
	}
	pageURL, err := url.Parse(flags.Arg(0))
	if err != nil || pageURL.Hostname() == "" || (pageURL.Scheme != "http" && pageURL.Scheme != "https") {
		return errors.New("invalid URL: expected an absolute HTTP or HTTPS URL")
	}
	cfg, err := readConfig(input)
	if err != nil {
		return err
	}
	ctx, cancel := context.WithTimeout(ctx, operationTimeout)
	defer cancel()
	client, err := newClient(cfg, pageURL)
	if err != nil {
		return err
	}
	defer client.CloseIdleConnections()
	pageHTML := []byte(cfg.HTML)
	if cfg.HTML == "" {
		pageHTML, pageURL, err = fetchPage(ctx, pageURL, cfg, client)
		if err != nil {
			return err
		}
	}
	article, err := readability.FromReader(bytes.NewReader(pageHTML), pageURL)
	if err != nil {
		return fmt.Errorf("extracting article: %w", err)
	}
	content, err := renderArticle(article, *plainText)
	if err != nil {
		return err
	}
	if *embedImgs && !*plainText {
		// Configured credentials remain scoped to the supplied URL, even after a redirect.
		origin, _ := url.Parse(flags.Arg(0))
		content, err = embedImages(ctx, content, client, cfg, origin)
		if err != nil {
			return fmt.Errorf("embedding images: %w", err)
		}
	}
	if err := ctx.Err(); err != nil {
		return err
	}
	_, err = io.WriteString(output, content)
	return err
}

func main() {
	var input io.Reader = os.Stdin
	if stat, err := os.Stdin.Stat(); err == nil && stat.Mode()&os.ModeCharDevice != 0 {
		input = strings.NewReader("")
	}
	if err := run(context.Background(), os.Args[1:], input, os.Stdout, os.Stderr); err != nil && !errors.Is(err, flag.ErrHelp) {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
