package main

import (
	"bytes"
	"compress/gzip"
	"context"
	"encoding/base64"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net"
	"net/http"
	"net/http/httptest"
	"net/url"
	"strings"
	"sync/atomic"
	"testing"
	"time"

	"codeberg.org/readeck/go-readability/v2"
	"golang.org/x/text/encoding/charmap"
	"golang.org/x/text/encoding/unicode"
)

var testPNG, _ = base64.StdEncoding.DecodeString("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+a5n8AAAAASUVORK5CYII=")

func articleHTML(extra string) string {
	return "<!doctype html><html><head><title>Regression article</title></head><body><article><h1>Regression article</h1><p>" +
		strings.Repeat("Readable article contents with enough words for extraction. ", 20) + "</p>" + extra + "</article></body></html>"
}

func configJSON(t *testing.T, cfg InputConfig) string {
	t.Helper()
	data, err := json.Marshal(cfg)
	if err != nil {
		t.Fatal(err)
	}
	return string(data)
}

func servePNG(w http.ResponseWriter) { w.Header().Set("Content-Type", "image/png"); w.Write(testPNG) }

func TestOriginMatching(t *testing.T) {
	for _, tc := range []struct {
		a, b string
		same bool
	}{
		{"https://example.com/a", "https://EXAMPLE.com:443/b", true},
		{"http://example.com", "http://example.com:80", true},
		{"https://bücher.example", "https://xn--bcher-kva.example", true},
		{"http://[::1]", "http://[::1]:80", true},
		{"http://[::1]", "http://[::2]", false},
		{"https://example.com", "http://example.com", false},
		{"https://example.com", "https://example.com:444", false},
		{"https://example.com", "https://other.example.com", false},
	} {
		t.Run(tc.a+"/"+tc.b, func(t *testing.T) {
			a, err := url.Parse(tc.a)
			if err != nil {
				t.Fatal(err)
			}
			b, err := url.Parse(tc.b)
			if err != nil {
				t.Fatal(err)
			}
			if sameOrigin(a, b) != tc.same {
				t.Fatalf("incorrect origin comparison for %s and %s", tc.a, tc.b)
			}
		})
	}
}

func TestRedirectUsesFinalBaseURL(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/old/story" {
			http.Redirect(w, r, "/old/story.xml.gz", 302)
			return
		}
		if r.URL.Path == "/old/story.xml.gz" {
			// Redirect bodies are not gzip data even when the URL or headers say gzip.
			w.Header().Set("Content-Encoding", "gzip")
			http.Redirect(w, r, "/new/path/story", 302)
			return
		}
		io.WriteString(w, articleHTML(`<a href="next.html">Related article</a><img src="photo.png">`))
	}))
	defer server.Close()
	output := runMain(t, []string{"extractor", server.URL + "/old/story"}, "")
	assertContains(t, output, server.URL+"/new/path/next.html")
	assertContains(t, output, server.URL+"/new/path/photo.png")
	if strings.Contains(output, server.URL+"/old/") {
		t.Fatal("original base URL survived redirect")
	}
}

func TestEmbeddingSharesPageCookies(t *testing.T) {
	var requests atomic.Int32
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/image" {
			requests.Add(1)
			cookie, err := r.Cookie("session")
			if err != nil || cookie.Value != "updated" {
				t.Errorf("missing updated cookie: %v, %v", cookie, err)
				http.Error(w, "unauthorized", 403)
				return
			}
			servePNG(w)
			return
		}
		http.SetCookie(w, &http.Cookie{Name: "session", Value: "updated", Path: "/"})
		io.WriteString(w, articleHTML(`<img src="/image">`))
	}))
	defer server.Close()
	cfg := InputConfig{Headers: map[string]string{"Cookie": "session=original; preference=keep"}}
	output := runMain(t, []string{"extractor", "-b", server.URL}, configJSON(t, cfg))
	assertContains(t, output, "data:image/png;base64,")
	if requests.Load() != 1 {
		t.Fatal("expected one image request")
	}
}

func TestConfiguredCookieDeletion(t *testing.T) {
	for _, deleteAt := range []string{"page", "redirect", "image"} {
		for _, expired := range []bool{false, true} {
			t.Run(fmt.Sprintf("%s/expired=%t", deleteAt, expired), func(t *testing.T) {
				var requests atomic.Int32
				server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					requests.Add(1)
					wantSession := r.URL.Path == "/page" || (deleteAt == "image" && r.URL.Path == "/first-image")
					cookie, err := r.Cookie("session")
					if wantSession && (err != nil || cookie.Value != "original") {
						t.Errorf("lost configured session on %s: %v, %v", r.URL.Path, cookie, err)
					} else if !wantSession && err != http.ErrNoCookie {
						t.Errorf("deleted session returned on %s: %v, %v", r.URL.Path, cookie, err)
					}
					if cookie, err := r.Cookie("preference"); err != nil || cookie.Value != "keep" {
						t.Errorf("lost unrelated configured cookie: %v, %v", cookie, err)
					}
					if (deleteAt != "image" && r.URL.Path == "/page") || r.URL.Path == "/first-image" {
						cookie := &http.Cookie{Name: "session", Path: "/", MaxAge: -1}
						if expired {
							cookie.MaxAge = 0
							cookie.Expires = time.Unix(1, 0)
						}
						http.SetCookie(w, cookie)
					}
					if deleteAt == "redirect" && r.URL.Path == "/page" {
						http.Redirect(w, r, "/final", http.StatusFound)
					} else if strings.HasSuffix(r.URL.Path, "image") {
						servePNG(w)
					} else {
						io.WriteString(w, articleHTML(`<img src="/first-image"><img src="/second-image">`))
					}
				}))
				defer server.Close()
				cfg := InputConfig{Headers: map[string]string{"Cookie": "session=original; preference=keep"}}
				output := runMain(t, []string{"extractor", "-b", server.URL + "/page"}, configJSON(t, cfg))
				if strings.Count(output, "data:image/png;base64,") != 2 || requests.Load() < 3 {
					t.Fatal("expected page and both image downloads")
				}
			})
		}
	}
}

func TestCredentialsStayOnConfiguredOrigin(t *testing.T) {
	for _, foreignHost := range []string{"localhost", "127.0.0.1"} {
		t.Run(foreignHost, func(t *testing.T) {
			for _, redirectedPage := range []bool{false, true} {
				t.Run(map[bool]string{false: "image-and-image-redirect", true: "page-redirect"}[redirectedPage], func(t *testing.T) {
					var requests atomic.Int32
					foreign := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
						requests.Add(1)
						for _, name := range []string{"Authorization", "Cookie", "X-Api-Key"} {
							if r.Header.Get(name) != "" {
								t.Errorf("forwarded %s to another origin", name)
							}
						}
						if r.Header.Get("User-Agent") != "Audit/1.0" || r.Header.Get("Accept-Language") != "cs" {
							t.Error("lost public headers")
						}
						if r.URL.Path == "/page" {
							io.WriteString(w, articleHTML(`<img src="/image">`))
							return
						}
						servePNG(w)
					}))
					defer foreign.Close()
					foreignURL := strings.Replace(foreign.URL, "127.0.0.1", foreignHost, 1)
					origin := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
						if r.Header.Get("Authorization") != "Bearer private" {
							t.Error("lost original-origin authorization")
						}
						if r.URL.Path == "/redirect-image" {
							http.Redirect(w, r, foreignURL+"/image", 302)
							return
						}
						if redirectedPage {
							http.Redirect(w, r, foreignURL+"/page", 302)
							return
						}
						io.WriteString(w, articleHTML(`<img src="`+foreignURL+`/image"><img src="/redirect-image">`))
					}))
					defer origin.Close()
					cfg := InputConfig{Headers: map[string]string{"Authorization": "Bearer private", "Cookie": "private=value", "X-Api-Key": "private", "User-Agent": "Audit/1.0", "Accept-Language": "cs"}}
					output := runMain(t, []string{"extractor", "-b", origin.URL}, configJSON(t, cfg))
					assertContains(t, output, "data:image/png;base64,")
					if requests.Load() != 2 {
						t.Fatalf("expected two foreign requests, got %d", requests.Load())
					}
				})
			}
		})
	}
}

func TestEmbeddingCachesImagesAndPreservesFailedAlternatives(t *testing.T) {
	var successful, failed atomic.Int32
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/failed" {
			failed.Add(1)
			http.Error(w, "unavailable", 404)
			return
		}
		successful.Add(1)
		servePNG(w)
	}))
	defer server.Close()
	origin, _ := url.Parse(server.URL)
	client, err := newClient(InputConfig{}, origin)
	if err != nil {
		t.Fatal(err)
	}
	defer client.CloseIdleConnections()
	markup := `<picture><source srcset="` + server.URL + `/alternative 2x"><img src="` + server.URL + `/ok" srcset="` + server.URL + `/alternative 2x"></picture>` +
		`<img src="` + server.URL + `/ok"><picture><source srcset="keep.jpg 2x"><img src="` + server.URL + `/failed" srcset="keep.jpg 2x"></picture><img src="` + server.URL + `/failed">`
	output, err := embedImages(context.Background(), markup, client, InputConfig{}, origin)
	if err != nil {
		t.Fatal(err)
	}
	if successful.Load() != 1 || failed.Load() != 1 {
		t.Fatalf("duplicate downloads: success %d, failure %d", successful.Load(), failed.Load())
	}
	if strings.Count(output, "data:image/png;base64,") != 2 {
		t.Fatal("expected two cached image occurrences")
	}
	if strings.Contains(output, "/alternative") {
		t.Fatal("responsive alternative overrides embedded src")
	}
	assertContains(t, output, `<source srcset="keep.jpg 2x"`)
	assertContains(t, output, `src="`+server.URL+`/failed" srcset="keep.jpg 2x"`)
}

func TestPlainTextIgnoresImageEmbedding(t *testing.T) {
	cfg := configJSON(t, InputConfig{HTML: articleHTML(`<img src="http://invalid.example/image">`)})
	plain := runMain(t, []string{"extractor", "-t", "https://example.com/article"}, cfg)
	combined := runMain(t, []string{"extractor", "-t", "-b", "https://example.com/article"}, cfg)
	if plain != combined || strings.Contains(combined, "<html>") {
		t.Fatal("-b changed plain-text output")
	}
}

func TestSVGAndInvalidImageResponses(t *testing.T) {
	for _, valid := range []bool{true, false} {
		t.Run(map[bool]string{true: "svg", false: "html-error"}[valid], func(t *testing.T) {
			server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				if valid {
					w.Header().Set("Content-Type", "image/svg+xml; charset=utf-8")
					io.WriteString(w, `<svg xmlns="http://www.w3.org/2000/svg"><circle r="5"/></svg>`)
				} else {
					w.Header().Set("Content-Type", "text/html")
					io.WriteString(w, "<html><body>Image unavailable</body></html>")
				}
			}))
			defer server.Close()
			origin, _ := url.Parse(server.URL)
			client, err := newClient(InputConfig{}, origin)
			if err != nil {
				t.Fatal(err)
			}
			defer client.CloseIdleConnections()
			image := downloadImage(context.Background(), server.URL, client, InputConfig{}, origin)
			if valid && !strings.HasPrefix(image, "data:image/svg+xml;base64,") {
				t.Fatal("SVG content type was not preserved")
			}
			if !valid && image != "" {
				t.Fatal("HTML error page was embedded as an image")
			}
		})
	}
}

func TestInvalidInputDoesNotFetchOrWriteOutput(t *testing.T) {
	var requests atomic.Int32
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { requests.Add(1); io.WriteString(w, articleHTML("")) }))
	defer server.Close()
	cases := []struct {
		name, input string
		args        []string
	}{
		{"malformed-json", "{broken", []string{server.URL}},
		{"wrong-type", `{"html":123}`, []string{server.URL}},
		{"trailing-json", `{} {}`, []string{server.URL}},
		{"empty-article", `{"html":"<html><body></body></html>"}`, []string{server.URL}},
		{"relative-url", "", []string{"relative"}},
		{"unsupported-scheme", "", []string{"file:///article"}},
		{"missing-url", "", nil},
		{"extra-url", "", []string{server.URL, server.URL}},
		{"invalid-proxy", `{"proxy":{"type":"http","address":"bad","password":"never-log-me"}}`, []string{server.URL}},
		{"unknown-proxy", `{"proxy":{"type":"unknown","address":"localhost:1234"}}`, []string{server.URL}},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			var output, stderr bytes.Buffer
			err := run(context.Background(), tc.args, strings.NewReader(tc.input), &output, &stderr)
			if err == nil || output.Len() != 0 {
				t.Fatalf("expected error and no output, got %v, %q", err, output.String())
			}
			if strings.Contains(err.Error(), "never-log-me") {
				t.Fatal("proxy password in error")
			}
		})
	}
	if requests.Load() != 0 {
		t.Fatal("invalid configuration triggered network activity")
	}
}

func TestHTTPFailureReturnsError(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { http.Error(w, "unavailable", 503) }))
	defer server.Close()
	var output, stderr bytes.Buffer
	if err := run(context.Background(), []string{server.URL}, strings.NewReader(""), &output, &stderr); err == nil || output.Len() != 0 {
		t.Fatalf("expected fetch error, got %v", err)
	}
}

func TestProxyEscapesCredentials(t *testing.T) {
	want := "Basic " + base64.StdEncoding.EncodeToString([]byte("audit@user:a#b:/@%"))
	var requests atomic.Int32
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		requests.Add(1)
		if r.Header.Get("Proxy-Authorization") != want {
			t.Error("proxy credentials were not encoded correctly")
		}
		io.WriteString(w, articleHTML(""))
	}))
	defer server.Close()
	proxyURL, _ := url.Parse(server.URL)
	cfg := InputConfig{Proxy: &ProxyConfig{Type: "http", Address: proxyURL.Host, Username: "audit@user", Password: "a#b:/@%"}}
	output := runMain(t, []string{"extractor", "-t", "http://article.invalid/story"}, configJSON(t, cfg))
	assertContains(t, output, "Readable article contents")
	if requests.Load() != 1 {
		t.Fatal("HTTP proxy was not used")
	}
}

func TestSOCKS5Authentication(t *testing.T) {
	listener, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatal(err)
	}
	defer listener.Close()
	result := make(chan error, 1)
	go func() {
		conn, err := listener.Accept()
		if err != nil {
			result <- err
			return
		}
		defer conn.Close()
		conn.SetDeadline(time.Now().Add(5 * time.Second))
		read := func(n int) ([]byte, error) { b := make([]byte, n); _, e := io.ReadFull(conn, b); return b, e }
		greeting, e := read(2)
		if e != nil {
			result <- e
			return
		}
		methods, e := read(int(greeting[1]))
		if e != nil {
			result <- e
			return
		}
		if !bytes.Contains(methods, []byte{2}) {
			result <- errors.New("missing password authentication")
			return
		}
		conn.Write([]byte{5, 2})
		auth, e := read(2)
		if e != nil {
			result <- e
			return
		}
		user, e := read(int(auth[1]))
		if e != nil {
			result <- e
			return
		}
		plen, e := read(1)
		if e != nil {
			result <- e
			return
		}
		password, e := read(int(plen[0]))
		if e != nil {
			result <- e
			return
		}
		if string(user) != "audit" || string(password) != "a#b" {
			result <- errors.New("wrong SOCKS5 credentials")
			return
		}
		conn.Write([]byte{1, 0})
		request, e := read(4)
		if e != nil {
			result <- e
			return
		}
		switch request[3] {
		case 1:
			_, e = read(4)
		case 3:
			var size []byte
			size, e = read(1)
			if e == nil {
				_, e = read(int(size[0]))
			}
		case 4:
			_, e = read(16)
		default:
			e = errors.New("invalid SOCKS5 address type")
		}
		if e != nil {
			result <- e
			return
		}
		if _, e = read(2); e != nil {
			result <- e
			return
		}
		conn.Write([]byte{5, 0, 0, 1, 127, 0, 0, 1, 0, 80})
		var header bytes.Buffer
		for !strings.HasSuffix(header.String(), "\r\n\r\n") {
			b, e := read(1)
			if e != nil {
				result <- e
				return
			}
			header.Write(b)
		}
		_, e = io.WriteString(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"+articleHTML(""))
		result <- e
	}()
	cfg := InputConfig{Proxy: &ProxyConfig{Type: "socks5", Address: listener.Addr().String(), Username: "audit", Password: "a#b"}}
	output := runMain(t, []string{"extractor", "-t", "http://article.invalid/story"}, configJSON(t, cfg))
	assertContains(t, output, "Readable article contents")
	if err := <-result; err != nil {
		t.Fatal(err)
	}
}

func TestNetworkCancellationStopsPageAndImages(t *testing.T) {
	for _, image := range []bool{false, true} {
		t.Run(map[bool]string{false: "page", true: "image"}[image], func(t *testing.T) {
			started := make(chan struct{})
			server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { close(started); <-r.Context().Done() }))
			defer server.Close()
			ctx, cancel := context.WithCancel(context.Background())
			defer cancel()
			go func() {
				select {
				case <-started:
					cancel()
				case <-ctx.Done():
				}
			}()
			cfg := ""
			args := []string{server.URL}
			if image {
				cfg = configJSON(t, InputConfig{HTML: articleHTML(`<img src="` + server.URL + `/image">`)})
				args = []string{"-b", server.URL}
			}
			var output, stderr bytes.Buffer
			err := run(ctx, args, strings.NewReader(cfg), &output, &stderr)
			if err == nil || !errors.Is(err, context.Canceled) || output.Len() != 0 {
				t.Fatalf("expected cancellation and no partial output: %v", err)
			}
		})
	}
}

func TestSizeLimits(t *testing.T) {
	if _, err := readLimited(strings.NewReader("1234"), 4); err != nil {
		t.Fatal(err)
	}
	if _, err := readLimited(strings.NewReader("12345"), 4); err == nil {
		t.Fatal("reader exceeded limit")
	}
	if _, err := readConfig(strings.NewReader(strings.Repeat(" ", maxConfigBytes+1))); err == nil {
		t.Fatal("configuration exceeded limit")
	}
	if _, err := readConfig(strings.NewReader(configJSON(t, InputConfig{HTML: strings.Repeat("x", maxHTMLBytes+1)}))); err == nil {
		t.Fatal("supplied HTML exceeded limit")
	}
	var output bytes.Buffer
	w := &limitedWriter{Writer: &output, remaining: 4}
	if _, err := w.Write([]byte("1234")); err != nil {
		t.Fatal(err)
	}
	if _, err := w.Write([]byte("5")); err == nil {
		t.Fatal("output exceeded limit")
	}
	if _, err := renderArticle(readability.Article{}, false); err == nil {
		t.Fatal("nil article node was accepted")
	}
}

func TestDownloadedPageLimit(t *testing.T) {
	for _, tc := range []struct {
		name, contentType, payload string
	}{
		{"utf8", "text/html; charset=utf-8", strings.Repeat("x", maxHTMLBytes+1)},
		// This shrinks below the limit during charset conversion, hiding Colly's truncation.
		{"utf16", "text/html; charset=utf-16le", strings.Repeat("x\x00", maxHTMLBytes/2+1)},
		// This fits before conversion, but expands beyond the limit in UTF-8.
		{"charset-expansion", "text/html; charset=windows-1250", strings.Repeat("\xe1", maxHTMLBytes/2+1)},
	} {
		t.Run(tc.name, func(t *testing.T) {
			server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				w.Header().Set("Content-Type", tc.contentType)
				io.WriteString(w, tc.payload)
			}))
			defer server.Close()
			var output, stderr bytes.Buffer
			err := run(context.Background(), []string{server.URL}, strings.NewReader(""), &output, &stderr)
			if err == nil || !strings.Contains(err.Error(), "downloaded HTML exceeds") || output.Len() != 0 {
				t.Fatalf("expected size error and no output: %v", err)
			}
		})
	}
}

func TestCompressedPageLimitStopsBeforeEOF(t *testing.T) {
	for _, acceptEncoding := range []string{"", "gzip"} {
		t.Run("accept-encoding="+acceptEncoding, func(t *testing.T) {
			done := make(chan struct{})
			server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				w.Header().Set("Content-Type", "text/html; charset=utf-8")
				w.Header().Set("Content-Encoding", "gzip")
				compressed := gzip.NewWriter(w)
				io.WriteString(compressed, strings.Repeat("x", maxHTMLBytes+1))
				compressed.Flush()
				w.(http.Flusher).Flush()
				// Deliberately leave the stream open: oversized decoded data must fail without EOF.
				<-done
				compressed.Close()
			}))
			defer server.Close()
			defer close(done)
			ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
			defer cancel()
			cfg := InputConfig{Headers: map[string]string{"Accept-Encoding": acceptEncoding}}
			var output, stderr bytes.Buffer
			err := run(ctx, []string{server.URL}, strings.NewReader(configJSON(t, cfg)), &output, &stderr)
			if err == nil || !strings.Contains(err.Error(), "downloaded HTML exceeds") || output.Len() != 0 {
				t.Fatalf("expected immediate decoded size error and no output: %v", err)
			}
		})
	}
}

func TestCompressedAndUTF16Pages(t *testing.T) {
	markup := articleHTML("<p>Complete article ending.</p>")
	utf16, err := unicode.UTF16(unicode.LittleEndian, unicode.IgnoreBOM).NewEncoder().String(markup)
	if err != nil {
		t.Fatal(err)
	}
	for _, tc := range []struct {
		name, contentType, encoding, path, payload string
	}{
		{"gzip", "text/html; charset=utf-8", "gzip", "/page", markup},
		{"gzip-content-type", "application/gzip", "", "/page", markup},
		{"gzip-suffix", "text/html; charset=utf-8", "", "/page.xml.gz", markup},
		{"utf16", "text/html; charset=utf-16le", "", "/page", utf16},
	} {
		t.Run(tc.name, func(t *testing.T) {
			server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				w.Header().Set("Content-Type", tc.contentType)
				w.Header().Set("Content-Encoding", tc.encoding)
				if strings.HasPrefix(tc.name, "gzip") {
					compressed := gzip.NewWriter(w)
					io.WriteString(compressed, tc.payload)
					compressed.Close()
				} else {
					io.WriteString(w, tc.payload)
				}
			}))
			defer server.Close()
			// Explicit encoding disables net/http's automatic gzip decompression.
			cfg := InputConfig{Headers: map[string]string{"Accept-Encoding": "gzip"}}
			output := runMain(t, []string{"extractor", "-t", server.URL + tc.path}, configJSON(t, cfg))
			assertContains(t, output, "Complete article ending.")
		})
	}
}

func TestImageSizeAndEmbeddingBudget(t *testing.T) {
	for _, oversized := range []bool{true, false} {
		t.Run(map[bool]string{true: "oversized", false: "total-budget"}[oversized], func(t *testing.T) {
			var requests atomic.Int32
			size := maxImageBytes
			if oversized {
				size++
			}
			payload := append(append([]byte{}, testPNG...), make([]byte, size-len(testPNG))...)
			server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				requests.Add(1)
				w.Header().Set("Content-Type", "image/png")
				w.Write(payload)
			}))
			defer server.Close()
			origin, _ := url.Parse(server.URL)
			client, err := newClient(InputConfig{}, origin)
			if err != nil {
				t.Fatal(err)
			}
			defer client.CloseIdleConnections()
			markup := strings.Repeat(`<img src="`+server.URL+`/image">`, 4)
			output, err := embedImages(context.Background(), markup, client, InputConfig{}, origin)
			if err != nil {
				t.Fatal(err)
			}
			count := strings.Count(output, "data:image/png;base64,")
			if oversized && count != 0 {
				t.Fatal("oversized image was embedded")
			}
			if !oversized && (count != 2 || !strings.Contains(output, server.URL+"/image")) {
				t.Fatal("total embedding budget was not respected")
			}
			if requests.Load() != 1 {
				t.Fatal("image was not cached")
			}
		})
	}
}

func TestLargeOutputAndNonUTF8Page(t *testing.T) {
	large := strings.Repeat("Readable large article text. ", 3000)
	cfg := configJSON(t, InputConfig{HTML: articleHTML("<p>" + large + "</p>")})
	output := runMain(t, []string{"extractor", "-t", "https://example.com/article"}, cfg)
	if len(output) < len(large) {
		t.Fatal("large output was truncated")
	}
	encoded, err := charmap.Windows1250.NewEncoder().String(articleHTML("<p>Příliš žluťoučký kůň.</p>"))
	if err != nil {
		t.Fatal(err)
	}
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/html; charset=windows-1250")
		io.WriteString(w, encoded)
	}))
	defer server.Close()
	output = runMain(t, []string{"extractor", "-t", server.URL}, "")
	assertContains(t, output, "Příliš žluťoučký kůň.")
}
