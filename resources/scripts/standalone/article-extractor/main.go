package main

import (
	"encoding/base64"
	"flag"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"os"
	"strings"

	"github.com/go-shiori/go-readability"
	"golang.org/x/net/html"
)

// embedImages replaces all <img src="URL"> with base64-embedded images
func embedImages(htmlContent string) (string, error) {
	doc, err := html.Parse(strings.NewReader(htmlContent))
	if err != nil {
		return "", err
	}

	var traverse func(*html.Node)
	traverse = func(n *html.Node) {
		if n.Type == html.ElementNode && n.Data == "img" {
			for i, attr := range n.Attr {
				if attr.Key == "src" && (strings.HasPrefix(attr.Val, "http://") || strings.HasPrefix(attr.Val, "https://")) {
					resp, err := http.Get(attr.Val)
					if err != nil {
						continue
					}
					defer resp.Body.Close()
					bytes, _ := io.ReadAll(resp.Body)
					mimeType := http.DetectContentType(bytes)
					base64Str := base64.StdEncoding.EncodeToString(bytes)
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
		fmt.Println("Usage: article-downloader [options] <url>")
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
	resp, err := http.Get(urlStr)
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
	if *plainText {
		output = article.TextContent
	} else {
		output = article.Content
		if *embedImgs {
			output, err = embedImages(output)
			if err != nil {
				fmt.Fprintf(os.Stderr, "Error embedding images: %v\n", err)
				os.Exit(1)
			}
		}
	}

	// Print result to stdout
	fmt.Printf("Title: %s\n\n%s", article.Title, output)
}
