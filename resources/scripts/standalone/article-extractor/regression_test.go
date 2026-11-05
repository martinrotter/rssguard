package main

import (
	"bytes"
	"context"
	"encoding/base64"
	"encoding/json"
	"errors"
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

func TestCredentialsStayOnConfiguredOrigin(t *testing.T) {
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
			foreignURL := strings.Replace(foreign.URL, "127.0.0.1", "localhost", 1)
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
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { io.WriteString(w, strings.Repeat("x", maxHTMLBytes+1)) }))
	defer server.Close()
	var output, stderr bytes.Buffer
	if err := run(context.Background(), []string{server.URL}, strings.NewReader(""), &output, &stderr); err == nil || output.Len() != 0 {
		t.Fatal("oversized page was accepted")
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
