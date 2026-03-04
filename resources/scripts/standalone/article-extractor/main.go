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

	"github.com/gocolly/colly/v2"

	"codeberg.org/readeck/go-readability/v2"
	"golang.org/x/net/html"
)

var httpClient = &http.Client{
	Timeout: 30 * time.Second,
}

// embedImages replaces all <img src="URL"> with base64 embedded images.
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
					(strings.HasPrefix(attr.Val, "http://") ||
						strings.HasPrefix(attr.Val, "https://")) {

					resp, err := httpClient.Get(attr.Val)
					if err != nil {
						continue
					}

					if resp.StatusCode < 200 || resp.StatusCode >= 300 {
						resp.Body.Close()
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

	var pageHTML []byte

	collector := colly.NewCollector(
		colly.UserAgent("curl/7.54"),
	)

	collector.SetRequestTimeout(30 * time.Second)

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

	article, err := readability.FromReader(bytes.NewReader(pageHTML), parsedURL)
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
