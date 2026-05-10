package main

import (
	"flag"
	"io"
	"net/http"
	"net/http/httptest"
	"os"
	"strings"
	"sync/atomic"
	"testing"
)

func TestMainUsesConfiguredHTMLAndSkipsFetch(t *testing.T) {
	var requests int32

	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		atomic.AddInt32(&requests, 1)
		http.Error(w, "configured HTML path should not fetch", http.StatusInternalServerError)
	}))
	defer server.Close()

	cfg := `{"html":"<!doctype html><html><head><title>Configured title</title></head><body><article><h1>Configured headline</h1><p>Configured body from stdin.</p></article></body></html>"}`

	output := runMain(t, []string{"rssguard-article-extractor", "-t", server.URL + "/article"}, cfg)

	if atomic.LoadInt32(&requests) != 0 {
		t.Fatalf("expected configured HTML to skip network fetch, got %d request(s)", requests)
	}

	assertContains(t, output, "Configured headline")
	assertContains(t, output, "Configured body from stdin.")
}

func TestMainFetchesURLWhenConfiguredHTMLIsMissing(t *testing.T) {
	var requests int32

	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		atomic.AddInt32(&requests, 1)

		if got := r.Header.Get("User-Agent"); got != "RssGuardTest/1.0" {
			t.Fatalf("expected configured user agent, got %q", got)
		}

		if got := r.Header.Get("X-Article-Test"); got != "fetch" {
			t.Fatalf("expected configured custom header, got %q", got)
		}

		w.Header().Set("Content-Type", "text/html; charset=utf-8")
		io.WriteString(w, "<!doctype html><html><head><title>Fetched title</title></head><body><article><h1>Fetched headline</h1><p>Fetched body from server.</p></article></body></html>")
	}))
	defer server.Close()

	cfg := `{"headers":{"User-Agent":"RssGuardTest/1.0","X-Article-Test":"fetch"}}`

	output := runMain(t, []string{"rssguard-article-extractor", "-t", server.URL + "/article"}, cfg)

	if atomic.LoadInt32(&requests) != 1 {
		t.Fatalf("expected one network fetch, got %d request(s)", requests)
	}

	assertContains(t, output, "Fetched headline")
	assertContains(t, output, "Fetched body from server.")
}

func TestMainEmbedsRemoteImagesAsBase64(t *testing.T) {
	pixelPNG := []byte{
		0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
		0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
		0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
		0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53,
		0xde, 0x00, 0x00, 0x00, 0x0c, 0x49, 0x44, 0x41,
		0x54, 0x08, 0xd7, 0x63, 0xf8, 0xcf, 0xc0, 0x00,
		0x00, 0x03, 0x01, 0x01, 0x00, 0xc9, 0xfe, 0x92,
		0xef, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e,
		0x44, 0xae, 0x42, 0x60, 0x82,
	}

	var imageRequests int32
	var server *httptest.Server

	server = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		switch r.URL.Path {
		case "/article":
			w.Header().Set("Content-Type", "text/html; charset=utf-8")
			io.WriteString(w, `<!doctype html><html><head><title>Image title</title></head><body><article><h1>Image headline</h1><p>Image body.</p><img src="`+server.URL+`/pixel.png"></article></body></html>`)
		case "/pixel.png":
			atomic.AddInt32(&imageRequests, 1)

			if got := r.Header.Get("User-Agent"); got != "RssGuardImageTest/1.0" {
				t.Fatalf("expected configured image user agent, got %q", got)
			}

			w.Header().Set("Content-Type", "image/png")
			w.Write(pixelPNG)
		default:
			http.NotFound(w, r)
		}
	}))
	defer server.Close()

	cfg := `{"headers":{"User-Agent":"RssGuardImageTest/1.0"}}`

	output := runMain(t, []string{"rssguard-article-extractor", "-b", server.URL + "/article"}, cfg)

	if atomic.LoadInt32(&imageRequests) != 1 {
		t.Fatalf("expected one image download, got %d request(s)", imageRequests)
	}

	assertContains(t, output, "data:image/png;base64,")

	if strings.Contains(output, server.URL+"/pixel.png") {
		t.Fatalf("expected remote image URL to be replaced, got %q", output)
	}
}

func runMain(t *testing.T, args []string, stdin string) string {
	t.Helper()

	oldArgs := os.Args
	oldStdin := os.Stdin
	oldStdout := os.Stdout
	oldFlagCommandLine := flag.CommandLine

	stdinFile, err := os.CreateTemp(t.TempDir(), "stdin-*.json")
	if err != nil {
		t.Fatal(err)
	}

	if _, err := stdinFile.WriteString(stdin); err != nil {
		t.Fatal(err)
	}

	if _, err := stdinFile.Seek(0, io.SeekStart); err != nil {
		t.Fatal(err)
	}

	stdoutReader, stdoutWriter, err := os.Pipe()
	if err != nil {
		t.Fatal(err)
	}

	os.Args = args
	os.Stdin = stdinFile
	os.Stdout = stdoutWriter
	flag.CommandLine = flag.NewFlagSet(args[0], flag.ExitOnError)

	defer func() {
		os.Args = oldArgs
		os.Stdin = oldStdin
		os.Stdout = oldStdout
		flag.CommandLine = oldFlagCommandLine
		stdinFile.Close()
		stdoutReader.Close()
	}()

	main()

	if err := stdoutWriter.Close(); err != nil {
		t.Fatal(err)
	}

	output, err := io.ReadAll(stdoutReader)
	if err != nil {
		t.Fatal(err)
	}

	return string(output)
}

func assertContains(t *testing.T, haystack string, needle string) {
	t.Helper()

	if !strings.Contains(haystack, needle) {
		t.Fatalf("expected output to contain %q, got %q", needle, haystack)
	}
}
