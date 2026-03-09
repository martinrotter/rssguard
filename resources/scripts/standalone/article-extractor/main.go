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
	"golang.org/x/net/proxy"
)

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

	// read JSON config
	var cfg InputConfig

	if fi, _ := os.Stdin.Stat(); fi.Size() > 0 {
		stdinData, _ := io.ReadAll(os.Stdin)
		json.Unmarshal(stdinData, &cfg)
	}

	var pageHTML []byte

	collector := colly.NewCollector(
		colly.UserAgent("curl/7.54"),
	)

	collector.SetRequestTimeout(30 * time.Second)

	// apply headers
	if len(cfg.Headers) > 0 {
		collector.OnRequest(func(r *colly.Request) {
			for _, h := range cfg.Headers {
				for k, v := range h {
					r.Headers.Set(k, v)
				}
			}
		})
	}

	// http client for images
	imageClient := &http.Client{
		Timeout: 30 * time.Second,
	}

	// proxy setup
	if cfg.Proxy != nil && cfg.Proxy.Address != "" {

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

			err := collector.SetProxy(proxyURL)
			if err != nil {
				fmt.Fprintf(os.Stderr, "Proxy error: %v\n", err)
				os.Exit(1)
			}

			u, _ := url.Parse(proxyURL)

			imageClient.Transport = &http.Transport{
				Proxy: http.ProxyURL(u),
			}

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

	imageMap := map[string]string{}

	// download images
	if *embedImgs {

		collector.OnHTML("img[src]", func(e *colly.HTMLElement) {
			src := e.Attr("src")
			resolvedSrc := src

			if !strings.HasPrefix(src, "http") {
				resolvedSrc = e.Request.AbsoluteURL(src)
			}

			if _, ok := imageMap[src]; ok {
				return
			}

			resp, err := imageClient.Get(resolvedSrc)
			if err != nil {
				return
			}

			data, err := io.ReadAll(resp.Body)
			resp.Body.Close()
			if err != nil {
				return
			}

			mime := http.DetectContentType(data)

			base64Str := base64.StdEncoding.EncodeToString(data)

			imageMap[src] = "data:" + mime + ";base64," + base64Str
		})
	}

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

	err = collector.Visit(urlStr)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error visiting URL: %v\n", err)
		os.Exit(1)
	}

	htmlStr := string(pageHTML)

	// replace image URLs
	if *embedImgs {

		for k, v := range imageMap {
			htmlStr = strings.ReplaceAll(htmlStr, k, v)
		}
	}

	article, err := readability.FromReader(strings.NewReader(htmlStr), parsedURL)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error parsing article: %v\n", err)
		os.Exit(1)
	}

	var output string
	var content bytes.Buffer

	if *plainText {

		article.RenderText(&content)
		output = content.String()

	} else {

		article.RenderHTML(&content)
		output = content.String()
	}

	fmt.Print(output)
}
