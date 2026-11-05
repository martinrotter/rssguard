"""Exercise the extractor's real CMake rules with a tiny, offline Go package."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


@unittest.skipUnless(all(shutil.which(tool) for tool in ("cmake", "ninja", "go")),
                     "Go, CMake and Ninja are required")
class ExtractorBuildTest(unittest.TestCase):
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

            def invoke(*args):
                result = subprocess.run(args, capture_output=True, text=True, timeout=120)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                return result.stdout

            def builds():
                return len(log.read_text().splitlines()) if log.exists() else 0

            def rebuild():
                invoke("cmake", "--build", str(build), "--target", "rssguard_article_extractor")

            invoke("cmake", "-G", generator, "-S", str(source), "-B", str(build),
                   "-DGO_EXECUTABLE=" + wrapper.as_posix(), "-DCMAKE_INSTALL_PREFIX=" + install.as_posix())
            rebuild()
            self.assertEqual(builds(), 1)
            executable = build / "src/rssguard" / ("rssguard-article-extractor" + executable_suffix)
            self.assertEqual(invoke(str(executable)), "first")
            rebuild()
            self.assertEqual(builds(), 1, "an unchanged build reran Go")

            extra.write_text('package main\nfunc value() string { return "updated" }\n', encoding="utf-8")
            rebuild()
            self.assertEqual(builds(), 2)
            self.assertEqual(invoke(str(executable)), "updated", "go build ignored another source file")

            # Manifest, vendor and compiler changes must each invalidate the custom command.
            for dependency in (helper / "go.mod", helper / "go.sum", vendor / "modules.txt", wrapper):
                previous = builds()
                with dependency.open("a", encoding="utf-8") as output:
                    output.write("\n")
                rebuild()
                self.assertEqual(builds(), previous + 1, str(dependency))

            # A compiler update must also be detected when its timestamp is preserved.
            previous = builds()
            timestamp = wrapper.stat()
            if os.name == "nt":
                text = wrapper.read_text()
                text = text.replace('@echo off\n', '@echo off\nif "%~1 %~2"=="env GOVERSION" (echo go1.27.999&exit /b 0)\n')
            else:
                text = wrapper.read_text().replace('#!/bin/sh\n', '#!/bin/sh\nif [ "$1 $2" = "env GOVERSION" ]; then echo go1.27.999; exit 0; fi\n')
            wrapper.write_text(text, encoding="utf-8")
            os.utime(wrapper, ns=(timestamp.st_atime_ns, timestamp.st_mtime_ns))
            rebuild()
            self.assertEqual(builds(), previous + 1, "compiler version changed without a rebuild")

            previous = builds()
            (helper / "added.go").write_text('package main\nfunc newlyAdded() {}\n', encoding="utf-8")
            rebuild()
            self.assertEqual(builds(), previous + 1, "new Go files did not invalidate the build")
            previous = builds()
            (helper / "added.go").unlink()
            rebuild()
            self.assertEqual(builds(), previous + 1, "removed Go files did not invalidate the build")
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
