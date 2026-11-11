# Compatibility source provenance

* Qt supermodule: `bab1fecd556ea561c4a89686293116741acfa1b4` (6.8.3).
* qtbase backport: crystalidea/qt6windows7
  `7cd5e9532e1af0e62a3bfe412d03fb725c6378ed` (matching 6.8.3 release).
* WebEngine backport: crystalidea/qt6windows7 WebEngine sources at
  `fb355ed9ecc7c1667084db2dd62508b3801cd85f` (same WebEngine files as
  `12d31c9e349dbf3c1d0d14497db1f3b41a113007`).
* WebEngine patch base: qt/qtwebengine
  `b586c4eb65d8e46ab2c255e1a141676043a650da` (v6.8.3), with
  qt/qtwebengine-chromium `55749ed0af5869215b88007df0cba430746583ae`.

`qtwebengine-6.8.3.patch` is the explicit unified diff between those exact
6.8.3 files and the 43 selected compatibility source files. It was checked
with `git apply --check` against the original files. There are no unrelated
Qt module or binary replacements. Retain the upstream per-file copyright and
license notices when distributing corresponding source.

The changes restore older-Windows fallbacks for Chromium randomness, WinRT,
DirectWrite, process/thread and DPI APIs, graphics, shared memory, proxy
resolution, capture and sandbox startup. The renderer and sandbox patches
are necessary as well as import fallbacks: merely loading QtWebEngineCore
successfully is insufficient. Applicability is verified; a native build and
rendering on Windows 8.1 are still required to establish runtime compatibility.

The source patch retains the original files' Chromium/third-party license
terms. Qt Community LGPL/GPL obligations apply to Qt and its backport.
QXmpp is LGPL-2.1-or-later (optional OpenSSL-based QXmpp encryption is off;
XMPP transport TLS remains enabled through Qt Network). OpenSSL is built from
`c10e643222ce5a185f2980d7352d545afacb0fed` (3.0.18), Apache-2.0.
Go and its compatibility patches retain their BSD-style notices. Build output
includes component notices and exact source pins. Preserve the corresponding
source and modification patch alongside any distributed binaries.
