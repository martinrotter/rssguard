package main

import (
	"bytes"
	"encoding/base64"
	"flag"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"os"
	"strings"
	"time"

	"codeberg.org/readeck/go-readability/v2"
	"golang.org/x/net/html"
)

// HTTP client with timeout
var client = &http.Client{
	Timeout: 30 * time.Second,
}

// fetchURL downloads a URL and validates HTTP status
func fetchURL(u string) (*http.Response, error) {
	req, err := http.NewRequest("GET", u, nil)
	if err != nil {
		return nil, err
	}

	req.Header.Set("User-Agent", "curl/7.54")

	resp, err := client.Do(req)
	if err != nil {
		return nil, err
	}

	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		resp.Body.Close()
		return nil, fmt.Errorf("unexpected HTTP status: %s", resp.Status)
	}

	return resp, nil
}

// embedImages replaces all <img src="URL"> with base64 embedded images
func embedImages(htmlContent string) (string, error) {
	doc, err := html.Parse(strings.NewReader(htmlContent))
	if err != nil {
		return "", err
	}

	var traverse func(*html.Node)
	traverse = func(n *html.Node) {
		if n.Type == html.ElementNode && n.Data == "img" {
			for i, attr := range n.Attr {
				if attr.Key == "src" &&
					(strings.HasPrefix(attr.Val, "http://") || strings.HasPrefix(attr.Val, "https://")) {

					resp, err := fetchURL(attr.Val)
					if err != nil {
						continue
					}

					data, err := io.ReadAll(resp.Body)
					resp.Body.Close()
					if err != nil {
						continue
					}

					mimeType := http.DetectContentType(data)
					base64Str := base64.StdEncoding.EncodeToString(data)

					n.Attr[i].Val = "data:" + mimeType + ";base64," + base64Str
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
	// CLI flags
	plainText := flag.Bool("t", false, "Output plain text instead of HTML")
	embedImgs := flag.Bool("b", false, "Embed images as base64")
	flag.Parse()

	if flag.NArg() < 1 {
		fmt.Println("Usage: rssguard-article-extractor [options] <url>")
		flag.PrintDefaults()
		os.Exit(1)
	}

	urlStr := flag.Arg(0)

	// Parse URL
	parsedURL, err := url.Parse(urlStr)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Invalid URL: %v\n", err)
		os.Exit(1)
	}

	// Download webpage
	resp, err := fetchURL(urlStr)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error fetching URL: %v\n", err)
		os.Exit(1)
	}
	defer resp.Body.Close()

	// Parse article
	article, err := readability.FromReader(resp.Body, parsedURL)
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

		if *embedImgs {
			output, err = embedImages(output)
			if err != nil {
				fmt.Fprintf(os.Stderr, "Error embedding images: %v\n", err)
				os.Exit(1)
			}
		}
	}

	fmt.Print(output)
}
