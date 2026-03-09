package main

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"os"
	"strings"
	"time"

	"github.com/gocolly/colly/v2"

	"codeberg.org/readeck/go-readability/v2"
	"golang.org/x/net/html"
	"golang.org/x/net/proxy"
)

const DefaultUserAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0 Safari/537.36"

type Header map[string]string

type ProxyConfig struct {
	Type     string `json:"type"`
	Address  string `json:"address"`
	Username string `json:"username"`
	Password string `json:"password"`
}

type InputConfig struct {
	Headers []Header     `json:"headers"`
	Proxy   *ProxyConfig `json:"proxy"`
}

func readConfigFromStdin() InputConfig {
	var cfg InputConfig

	stat, _ := os.Stdin.Stat()

	if (stat.Mode() & os.ModeCharDevice) == 0 {
		stdinData, _ := io.ReadAll(os.Stdin)
		if len(stdinData) > 0 {
			json.Unmarshal(stdinData, &cfg)
		}
	}

	return cfg
}

func resolveUserAgent(cfg InputConfig) string {
	userAgent := DefaultUserAgent

	for _, h := range cfg.Headers {
		if ua, ok := h["User-Agent"]; ok {
			userAgent = ua
		}
	}

	return userAgent
}

func applyHeaders(r *colly.Request, headers []Header, userAgent string) {
	r.Headers.Set("User-Agent", userAgent)

	for _, h := range headers {
		for k, v := range h {
			r.Headers.Set(k, v)
		}
	}
}

func setupCollector(userAgent string, headers []Header) *colly.Collector {
	collector := colly.NewCollector(
		colly.UserAgent(userAgent),
	)

	collector.SetRequestTimeout(30 * time.Second)

	collector.OnRequest(func(r *colly.Request) {
		applyHeaders(r, headers, userAgent)
	})

	return collector
}

func setupProxy(cfg InputConfig, collector *colly.Collector, imageClient *http.Client) {
	if cfg.Proxy == nil || cfg.Proxy.Address == "" {
		return
	}

	switch strings.ToLower(cfg.Proxy.Type) {
	case "http":
		proxyURL := "http://" + cfg.Proxy.Address

		if cfg.Proxy.Username != "" {
			proxyURL = fmt.Sprintf(
				"http://%s:%s@%s",
				cfg.Proxy.Username,
				cfg.Proxy.Password,
				cfg.Proxy.Address,
			)
		}

		u, err := url.Parse(proxyURL)

		if err != nil {
			fmt.Fprintf(os.Stderr, "Proxy parse error: %v\n", err)
			os.Exit(1)
		}

		transport := &http.Transport{
			Proxy: http.ProxyURL(u),
		}

		collector.WithTransport(transport)
		imageClient.Transport = transport

	case "socks5":
		var auth *proxy.Auth

		if cfg.Proxy.Username != "" {
			auth = &proxy.Auth{
				User:     cfg.Proxy.Username,
				Password: cfg.Proxy.Password,
			}
		}

		dialer, err := proxy.SOCKS5(
			"tcp",
			cfg.Proxy.Address,
			auth,
			proxy.Direct,
		)

		if err != nil {
			fmt.Fprintf(os.Stderr, "SOCKS5 proxy error: %v\n", err)
			os.Exit(1)
		}

		transport := &http.Transport{
			Dial: dialer.Dial,
		}

		collector.WithTransport(transport)
		imageClient.Transport = transport
	}
}

func fetchPage(urlStr string, collector *colly.Collector) []byte {
	var pageHTML []byte

	collector.OnResponse(func(r *colly.Response) {
		if r.StatusCode < 200 || r.StatusCode >= 300 {
			fmt.Fprintf(os.Stderr, "HTTP error: %d\n", r.StatusCode)
			os.Exit(1)
		}

		pageHTML = r.Body
	})

	collector.OnError(func(r *colly.Response, err error) {
		fmt.Fprintf(os.Stderr, "Request failed: %v\n", err)
		os.Exit(1)
	})

	err := collector.Visit(urlStr)

	if err != nil {
		fmt.Fprintf(os.Stderr, "Error visiting URL: %v\n", err)
		os.Exit(1)
	}

	return pageHTML
}

func extractArticle(pageHTML []byte, parsedURL *url.URL) readability.Article {
	article, err := readability.FromReader(bytes.NewReader(pageHTML), parsedURL)

	if err != nil {
		fmt.Fprintf(os.Stderr, "Error parsing article: %v\n", err)
		os.Exit(1)
	}

	return article
}

func renderArticle(article readability.Article, plainText bool) string {
	var content bytes.Buffer

	if plainText {
		article.RenderText(&content)
	} else {
		article.RenderHTML(&content)
	}

	return content.String()
}

func downloadImage(src string, client *http.Client, headers []Header, userAgent string) string {
	req, err := http.NewRequest("GET", src, nil)

	if err != nil {
		return ""
	}

	req.Header.Set("User-Agent", userAgent)

	for _, h := range headers {
		for k, v := range h {
			req.Header.Set(k, v)
		}
	}

	resp, err := client.Do(req)

	if err != nil {
		return ""
	}

	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		resp.Body.Close()
		return ""
	}

	data, err := io.ReadAll(resp.Body)
	resp.Body.Close()

	if err != nil {
		return ""
	}

	mimeType := http.DetectContentType(data)
	base64Str := base64.StdEncoding.EncodeToString(data)

	return "data:" + mimeType + ";base64," + base64Str
}

func embedImages(htmlContent string, client *http.Client, headers []Header, userAgent string) (string, error) {
	doc, err := html.Parse(strings.NewReader(htmlContent))

	if err != nil {
		return "", err
	}

	var traverse func(*html.Node)

	traverse = func(n *html.Node) {
		if n.Type == html.ElementNode && n.Data == "img" {
			for i, attr := range n.Attr {
				if attr.Key == "src" &&
					(strings.HasPrefix(attr.Val, "http://") ||
						strings.HasPrefix(attr.Val, "https://")) {

					dataURI := downloadImage(attr.Val, client, headers, userAgent)

					if dataURI != "" {
						n.Attr[i].Val = dataURI
					}
				}
			}
		}

		for c := n.FirstChild; c != nil; c = c.NextSibling {
			traverse(c)
		}
	}

	traverse(doc)

	var b strings.Builder
	html.Render(&b, doc)

	return b.String(), nil
}

func main() {

	plainText := flag.Bool("t", false, "Output plain text instead of HTML")
	embedImgs := flag.Bool("b", false, "Embed images as base64")

	flag.Parse()

	if flag.NArg() < 1 {
		fmt.Println("Usage: rssguard-article-extractor [options] <url>")
		flag.PrintDefaults()
		os.Exit(1)
	}

	urlStr := flag.Arg(0)
	parsedURL, err := url.Parse(urlStr)

	if err != nil {
		fmt.Fprintf(os.Stderr, "Invalid URL: %v\n", err)
		os.Exit(1)
	}

	cfg := readConfigFromStdin()
	userAgent := resolveUserAgent(cfg)

	imageClient := &http.Client{
		Timeout: 30 * time.Second,
	}

	collector := setupCollector(userAgent, cfg.Headers)

	setupProxy(cfg, collector, imageClient)

	pageHTML := fetchPage(urlStr, collector)
	article := extractArticle(pageHTML, parsedURL)
	output := renderArticle(article, *plainText)

	if *embedImgs {
		output, err = embedImages(output, imageClient, cfg.Headers, userAgent)

		if err != nil {
			fmt.Fprintf(os.Stderr, "Error embedding images: %v\n", err)
			os.Exit(1)
		}
	}

	fmt.Print(output)
}
