"""Exercise the extractor's real CMake rules with a tiny, offline Go package."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest


@unittest.skipUnless(all(shutil.which(tool) for tool in ("cmake", "ninja", "go")),
                     "Go, CMake and Ninja are required")
class ExtractorBuildTest(unittest.TestCase):
    @unittest.skipUnless(all(shutil.which(tool) for tool in ("git", "gofmt")), "Git and gofmt are required")
    def test_crlf_checkout(self):
        with tempfile.TemporaryDirectory(prefix="rssguard-extractor-crlf-") as directory:
            root = Path(directory)
            package = Path(__file__).resolve().parent

            def invoke(*args):
                result = subprocess.run(args, cwd=root, capture_output=True, text=True, timeout=120)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                return result.stdout

            invoke("git", "init", "--quiet")
            invoke("git", "config", "core.autocrlf", "true")
            shutil.copyfile(package / ".gitattributes", root / ".gitattributes")
            files = [*package.glob("*.go"), package / "go.mod", package / "go.sum"]
            for path in files:
                (root / path.name).write_bytes(path.read_bytes().replace(b"\r\n", b"\n").replace(b"\n", b"\r\n"))
            # Uncovered files must actually use CRLF, proving the checkout configuration is active.
            (root / "control.txt").write_bytes(b"control\n")
            invoke("git", "add", ".")
            for path in [*files, Path("control.txt")]:
                (root / path.name).unlink()
            invoke("git", "checkout-index", "--all", "--force")
            self.assertIn(b"\r\n", (root / "control.txt").read_bytes())
            for path in files:
                self.assertNotIn(b"\r", (root / path.name).read_bytes(), path.name)
            self.assertEqual(invoke("gofmt", "-l", "."), "")
            invoke("go", "mod", "tidy", "-diff")

    def test_incremental_build_and_install(self):
        self.check_build("Ninja")

    @unittest.skipUnless(shutil.which("nmake" if os.name == "nt" else "make"), "Make is not available")
    def test_make_incremental_build_and_install(self):
        self.check_build("NMake Makefiles" if os.name == "nt" else "Unix Makefiles")

    def check_build(self, generator):
        with tempfile.TemporaryDirectory(prefix="rssguard-extractor-build-") as directory:
            root = Path(directory).resolve()
            self.assertEqual(root.parent, Path(tempfile.gettempdir()).resolve())
            source = root / "source"
            source.mkdir()
            helper = source / "extractor"
            helper.mkdir()
            shutil.copyfile(Path(__file__).with_name("CMakeLists.txt"), helper / "CMakeLists.txt")
            shutil.copyfile(Path(__file__).with_name("check_go.cmake"), helper / "check_go.cmake")
            (helper / "go.mod").write_text("module extractor-build-test\n\ngo 1.26.0\n", encoding="utf-8")
            (helper / "go.sum").write_text("", encoding="utf-8")
            (helper / "main.go").write_text('package main\nimport "fmt"\nfunc main() { fmt.Print(value()) }\n', encoding="utf-8")
            extra = helper / "value.go"
            extra.write_text('package main\nfunc value() string { return "first" }\n', encoding="utf-8")
            vendor = helper / "vendor"
            vendor.mkdir()
            (vendor / "modules.txt").write_text("", encoding="utf-8")
            executable_suffix = ".exe" if os.name == "nt" else ""
            (source / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.21)\nproject(extractor_build_test NONE)\n'
                f'set(CMAKE_EXECUTABLE_SUFFIX "{executable_suffix}")\n'
                'add_custom_target(rssguard)\nadd_subdirectory(extractor)\n', encoding="utf-8")
            log = root / "invocations.txt"
            go = Path(shutil.which("go")).resolve()
            wrapper = root / ("go-wrapper.cmd" if os.name == "nt" else "go-wrapper.sh")
            if os.name == "nt":
                wrapper.write_text(f'@echo off\nif "%~1"=="build" echo build>>"{log}"\n"{go}" %*\n', encoding="utf-8")
            else:
                import shlex
                wrapper.write_text('#!/bin/sh\n' +
                                   f'if [ "$1" = build ]; then echo build >> {shlex.quote(str(log))}; fi\n' +
                                   f'exec {shlex.quote(str(go))} "$@"\n', encoding="utf-8")
                wrapper.chmod(0o755)
            build = root / "build"
            install = root / "install"
            executable = build / "src/rssguard" / ("rssguard-article-extractor" + executable_suffix)
            last_build_finished_ns = 0
            last_build_output = ""

            def invoke(*args):
                result = subprocess.run(args, capture_output=True, text=True, timeout=120)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                return result.stdout

            def builds():
                return len(log.read_text().splitlines()) if log.exists() else 0

            def diagnostics():
                paths = [extra, helper / "go.mod", helper / "go.sum", vendor / "modules.txt", wrapper,
                         helper / "added.go", executable, build / "build.ninja",
                         build / "extractor/go-sources.txt", build / "extractor/go-version.txt"]
                lines = [f"Generator: {generator}", f"Last build finished (ns): {last_build_finished_ns}"]
                for path in paths:
                    if path.exists():
                        stat = path.stat()
                        lines.append(f"{path}: mtime_ns={stat.st_mtime_ns}, size={stat.st_size}")
                    else:
                        lines.append(f"{path}: missing")
                ninja_log = build / ".ninja_log"
                if ninja_log.exists():
                    lines.extend(["Ninja log:", *ninja_log.read_text().splitlines()[-20:]])
                lines.extend(["Build output:", last_build_output])
                return "\n".join(lines)

            def edit_dependency(path, contents):
                reference_ns = max(last_build_finished_ns,
                                   path.stat().st_mtime_ns if path.exists() else 0,
                                   executable.stat().st_mtime_ns if executable.exists() else 0)
                path.write_text(contents, encoding="utf-8")
                deadline = time.monotonic() + 5
                # Cross a whole-second boundary for generators/filesystems with coarse timestamps.
                # Touch with the real clock; future timestamps can cause CMake regeneration loops.
                while path.stat().st_mtime_ns // 1_000_000_000 <= reference_ns // 1_000_000_000:
                    if time.monotonic() >= deadline:
                        self.fail(f"Timestamp did not advance for {path}\n{diagnostics()}")
                    time.sleep(0.02)
                    os.utime(path, None)

            def rebuild():
                nonlocal last_build_finished_ns, last_build_output
                args = ["cmake", "--build", str(build), "--target", "rssguard_article_extractor", "--verbose"]
                if generator == "Ninja":
                    args.extend(["--", "-d", "explain"])
                result = subprocess.run(args, capture_output=True, text=True, timeout=120)
                last_build_finished_ns = time.time_ns()
                last_build_output = result.stdout + result.stderr
                self.assertEqual(result.returncode, 0, diagnostics())

            def assert_builds(expected, reason):
                actual = builds()
                self.assertEqual(actual, expected, f"{reason}\n{diagnostics()}" if actual != expected else reason)

            invoke("cmake", "-G", generator, "-S", str(source), "-B", str(build),
                   "-DGO_EXECUTABLE=" + wrapper.as_posix(), "-DCMAKE_INSTALL_PREFIX=" + install.as_posix())
            rebuild()
            assert_builds(1, "initial build")
            self.assertEqual(invoke(str(executable)), "first")
            rebuild()
            assert_builds(1, "an unchanged build reran Go")

            edit_dependency(extra, 'package main\nfunc value() string { return "updated" }\n')
            rebuild()
            assert_builds(2, "changed Go source")
            self.assertEqual(invoke(str(executable)), "updated", "go build ignored another source file")

            # Manifest, vendor and compiler changes must each invalidate the custom command.
            for dependency in (helper / "go.mod", helper / "go.sum", vendor / "modules.txt", wrapper):
                previous = builds()
                edit_dependency(dependency, dependency.read_text(encoding="utf-8") + "\n")
                rebuild()
                assert_builds(previous + 1, str(dependency))

            # A compiler update must also be detected when its timestamp is preserved.
            previous = builds()
            timestamp = wrapper.stat()
            if os.name == "nt":
                text = wrapper.read_text()
                text = text.replace('@echo off\n', '@echo off\nif "%~1 %~2"=="env GOVERSION" (echo go1.27.999&exit /b 0)\n')
            else:
                text = wrapper.read_text().replace('#!/bin/sh\n', '#!/bin/sh\nif [ "$1 $2" = "env GOVERSION" ]; then echo go1.27.999; exit 0; fi\n')
            edit_dependency(wrapper, text)
            os.utime(wrapper, ns=(timestamp.st_atime_ns, timestamp.st_mtime_ns))
            rebuild()
            assert_builds(previous + 1, "compiler version changed without a rebuild")

            previous = builds()
            edit_dependency(helper / "added.go", 'package main\nfunc newlyAdded() {}\n')
            rebuild()
            assert_builds(previous + 1, "new Go files did not invalidate the build")
            previous = builds()
            # Let the regenerated source-list manifest also get a newer timestamp after removal.
            edit_dependency(helper / "added.go", (helper / "added.go").read_text(encoding="utf-8"))
            (helper / "added.go").unlink()
            rebuild()
            assert_builds(previous + 1, "removed Go files did not invalidate the build")
            invoke("cmake", "--install", str(build))
            destination = "" if os.name == "nt" else ("Contents/MacOS" if sys.platform == "darwin" else "bin")
            installed = install / destination / executable.name
            self.assertEqual(invoke(str(installed)), "updated")

            wrapper.write_text(wrapper.read_text().replace("go1.27.999", "go1.25.0"), encoding="utf-8")
            rejected = subprocess.run(
                ["cmake", "-G", generator, "-S", str(source), "-B", str(root / "old-compiler"),
                 "-DGO_EXECUTABLE=" + wrapper.as_posix()], capture_output=True, text=True, timeout=120)
            self.assertNotEqual(rejected.returncode, 0)
            self.assertIn("requires Go 1.26.0", rejected.stdout + rejected.stderr)


if __name__ == "__main__":
    unittest.main()
