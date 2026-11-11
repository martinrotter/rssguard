# RSS Guard with XMPP on Windows 8.1 x64

This is a **candidate build pipeline**, not a claim that either Windows binary
has passed a Windows 8.1 runtime test. The Linux development environment cannot
execute Windows applications. A successful Actions build on Windows Server 2022
also does not establish Windows 8.1 compatibility.

## Two variants

The `Windows 8.1 XMPP candidates` workflow builds both:

* **text**: the existing QTextBrowser article viewer;
* **web**: Qt WebEngine, including the patched Chromium subprocess and sandbox.

Both contain the existing RSS Guard XMPP service plugin and the same PubSub
implementation. Neither includes the optional media player or XMPP OMEMO/Jingle.
SQLite is included; MySQL is outside these profiles. They use separate SDK,
build and packaging directories. Do not combine their files with another RSS
Guard installation, a stock Qt SDK, or any Qt 5 DLLs.

## Why Qt 6

The pinned QXmpp revision is 1.15.1 and requires Qt 6.4+. RSS Guard uses its new
discovery and task APIs; changing the Qt 5 build's feature flag would not port
those APIs. A Qt 5 solution would require maintaining an older QXmpp API port.

CrystalIDEA's [qt6windows7](https://github.com/crystalidea/qt6windows7) explicitly
supports Windows 8.1. Its newer release supports Qt 6.8.5, but the public Qt
source refs used here provide Qt 6.8.3. We therefore pin Qt 6.8.3 and its matching
qtbase patch set. The newer WebEngine fixes are ported as a reviewed source
patch against the exact 6.8.3 Chromium tree; see [PATCHES.md](PATCHES.md).
This approach still requires native Windows testing, especially the web
renderer, fonts, GPU fallback and sandbox. The build never disables the
Chromium sandbox to make a check pass.

All Qt modules are compiled together. QXmpp, the RSS Guard core, its plugins and
the validation executables use that single SDK and MSVC 14.38. Compatibility
changes are applied to source **before compilation**. `windeployqt` selects the
runtime libraries and plugins. No compatibility folder is copied into the app.

The extractor also needs attention: upstream Go 1.21+ no longer supports
Windows 8.1. The workflow rebuilds Go 1.26.11 with the pinned
[XTLS compatibility patch](https://github.com/XTLS/go-win7), runs its extractor
tests, and disables automatic toolchain replacement. Stock Go would undermine
compatibility even with patched Qt.

## Build in GitHub Actions

Commit these changes to your fork, open Actions, select **Windows 8.1 XMPP
candidates**, and run the workflow on that branch. It has separate `text` and
`web` artifacts. These are portable ZIPs named `candidate` until native testing
has passed. The existing Windows 7 and modern Windows release matrix is retained.

The source build uses substantial resources, especially Chromium. The workflow
allows six hours per variant. A hosted runner's available disk, memory or time
can limit the web build; inspect the retained CMake/test logs before treating a
failed build as a compatibility defect. A larger Windows build host can run the
same script without changing the package. This pipeline has not yet been run
on GitHub Actions from this development environment.

For a local modern Windows build, use an x64 VS 2022 developer shell with the
14.38 compiler component, CMake 3.21+, Ninja, Python 3, Perl, Node.js and a Go
1.26 bootstrap SDK on PATH. Use a short path with ample free space:

```powershell
./resources/scripts/windows81/build.ps1 -Work C:\rg81-text -Viewer text -Jobs 4
./resources/scripts/windows81/build.ps1 -Work C:\rg81-web -Viewer web -Jobs 4
```

Use fresh work directories when changing dependency pins or flags. Generated
packages are never overlaid. Dependencies are pinned by Git commit; Go sources
and Microsoft's redistributable are checksum verified. The package records the
source revisions and viewer in `build-manifest.json`.

## Test on your Windows 8.1 machine (no Actions runner required)

1. Use Windows 8.1 x64 with the April 2014 update (KB2919355) and Universal CRT
   update (KB2999226).
2. Extract each candidate into its own directory. Install the included Microsoft
   prerequisite `prerequisites\VC_redist.x64.exe` (14.38.33135) if a compatible
   Visual C++ runtime is not already installed. This is a normal prerequisite
   installation, not manual DLL replacement.
3. Open Windows PowerShell in the extracted directory and run:

   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\validate-windows81.ps1
   ```

The validator starts only packaged executables, strips SDK overrides, restricts
DLL search PATH, checks the Qt version, loads the deployed XMPP plugin, performs
an offline STARTTLS/SASL/discovery/PubSub exchange, verifies Atom payloads from
fetch and push events, rejects an untrusted certificate, renders an article in
the selected viewer and checks extractor startup. It reports failure on a
non-8.1 OS unless explicitly invoked for build-host checks. Preserve the
printed `result.json` and `validation.log` paths for **each** variant.

The offline fixture uses a public test-only localhost certificate and key. It
never connects to an external XMPP server or requires your account password.
The test-only certificate is trusted only by that test client; it does not
change your system trust store or disable production certificate validation.

Then launch `rssguard.exe` normally. Add your XMPP account, synchronize its
services and subscriptions, refresh an Atom node, publish a new item from
another client and verify it appears without a manual refresh. Restart RSS
Guard and check the account, node and articles persist. Compare both viewers,
including article links and rendering. This last test covers your actual
server, account permissions and database integration beyond the deterministic
protocol fixture. Account credentials are entered only in RSS Guard.

## Validation status

Update this section only with observed results. The package manifest deliberately
starts with `windows81Validated: false`; build-host tests never set it to true.
Native Windows builds, target-machine results and actual-server acceptance
remain required before calling either candidate fully functional on Windows 8.1.

Observed development checks (Linux, Qt 6.8.2 system SDK): both text and web
applications plus XMPP plugins compiled; the deployed-plugin check and the
PubSub fixture passed; each viewer rendered the smoke article and completed
normal application shutdown. The patched Go 1.26.11 SDK built, extractor tests
passed, and the extractor cross-compiled to Windows x64. PowerShell scripts
passed PowerShell 7 parser checks. These results exercise application code,
not the patched Windows Qt binaries or Windows PowerShell 5.1 execution.
