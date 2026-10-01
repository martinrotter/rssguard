"""Portable regression tests for macOS release validation (no macOS tools needed)."""

import importlib.util
import os
from pathlib import Path
import plistlib
import signal
import subprocess
import tempfile
import unittest
from unittest.mock import patch


script = Path(__file__).resolve().parents[1] / "validate-macos-package.py"
spec = importlib.util.spec_from_file_location("macos_package", script)
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)


def build_version(minimum="13.0", platform="1"):
    return f"Load command 0\n      cmd LC_BUILD_VERSION\n platform {platform}\n    minos {minimum}\n      sdk 15.0\n"


class CompatibilityTests(unittest.TestCase):
    def test_version_comparison_pads_components(self):
        self.assertEqual(package.version_tuple("13"), package.version_tuple("13.0.0"))
        self.assertLess(package.version_tuple("13.9"), package.version_tuple("13.10"))

    def test_invalid_version_is_rejected(self):
        for value in ("", "13.x", "13.0.0.1"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                package.version_tuple(value)

    def test_modern_and_legacy_deployment_commands(self):
        self.assertEqual(package.minimum_versions(build_version()), ["13.0"])
        legacy = "Load command 0\n cmd LC_VERSION_MIN_MACOSX\n version 12.0\n sdk 15.0\n"
        self.assertEqual(package.minimum_versions(legacy), ["12.0"])

    def test_named_macos_platform(self):
        self.assertEqual(package.minimum_versions(build_version(platform="MACOS")), ["13.0"])

    def test_other_platform_is_rejected(self):
        with self.assertRaises(RuntimeError):
            package.minimum_versions(build_version(platform="2"))

    def test_missing_deployment_metadata_is_rejected(self):
        with self.assertRaises(RuntimeError):
            package.minimum_versions("Load command 0\n cmd LC_UUID\n")

    def test_malformed_deployment_command_is_rejected(self):
        with self.assertRaises(RuntimeError):
            package.minimum_versions("Load command 0\n cmd LC_BUILD_VERSION\n platform 1\n")

    @patch.object(package, "run", side_effect=["arm64", build_version()])
    def test_native_slice_is_accepted(self, command):
        self.assertEqual(package.inspect_binary(Path("binary"), "arm64", "13.0"), [])

    @patch.object(package, "run", return_value="arm64")
    def test_arm_only_binary_is_rejected_for_intel(self, command):
        with self.assertRaisesRegex(RuntimeError, "expected x86_64"):
            package.inspect_binary(Path("binary"), "x86_64", "13.0")

    @patch.object(package, "run", side_effect=["x86_64 arm64", build_version()])
    def test_universal_dependency_is_accepted(self, command):
        package.inspect_binary(Path("binary"), "x86_64", "13.0")
        self.assertEqual(command.call_args.args[0][2], "x86_64")

    @patch.object(package, "run", side_effect=["x86_64h arm64", build_version()])
    def test_intel_webengine_subarchitecture_is_accepted(self, command):
        package.inspect_binary(Path("binary"), "x86_64", "13.0")
        self.assertEqual(command.call_args.args[0][2], "x86_64h")

    @patch.object(package, "run", side_effect=["arm64", build_version("15.0")])
    def test_newer_macos_dependency_is_rejected(self, command):
        with self.assertRaisesRegex(RuntimeError, "requires macOS 15.0"):
            package.inspect_binary(Path("binary"), "arm64", "13.0")

    def test_library_names_with_spaces_and_weak_dependencies(self):
        commands = ("Load command 1\n cmd LC_LOAD_WEAK_DYLIB\n"
                    " name @rpath/RSS Guard.framework/RSS Guard (offset 24)\n"
                    "Load command 2\n cmd LC_ID_DYLIB\n name /build/self.dylib (offset 24)\n")
        self.assertEqual(package.dependencies(commands), ["@rpath/RSS Guard.framework/RSS Guard"])


class CommandTests(unittest.TestCase):
    @patch.object(package.subprocess, "run", return_value=subprocess.CompletedProcess(["probe"], 0, "ok", ""))
    def test_successful_command_returns_output(self, command):
        self.assertEqual(package.run(["probe"]), "ok")

    @patch.object(package.subprocess, "run", return_value=subprocess.CompletedProcess(["probe"], 7, "", "failure"))
    def test_failure_reports_exit_code_and_stderr(self, command):
        with self.assertRaisesRegex(RuntimeError, "exit code 7") as error:
            package.run(["probe"])
        self.assertIn("failure", str(error.exception))

    @patch.object(package.subprocess, "run")
    def test_rendering_success_does_not_hide_shutdown_crash(self, command):
        command.return_value = subprocess.CompletedProcess(["probe"], -signal.SIGSEGV,
                                                          "RSSGUARD_PACKAGE_SMOKE_OK:web", "")
        with self.assertRaisesRegex(RuntimeError, "signal SIGSEGV"):
            package.run(["probe"])

    @patch.object(package.shutil, "which", return_value="lldb")
    @patch.object(package.subprocess, "run", return_value=subprocess.CompletedProcess(["lldb"], 0, "backtrace", ""))
    def test_debugger_uses_fresh_profile_and_captures_all_threads(self, command, which):
        with tempfile.TemporaryDirectory() as directory:
            environment = {"PATH": "normal-path"}
            package.diagnose_viewer_failure(Path("RSS Guard.app/Contents/MacOS/probe"), Path(directory), environment)
            self.assertTrue((Path(directory) / "debugger-profile/config").is_dir())
            arguments = command.call_args.args[0]
            self.assertIn("thread backtrace all", arguments)
            self.assertIn("process kill", arguments)
            self.assertEqual(arguments[arguments.index("--data") + 1], str(Path(directory) / "debugger-profile"))
            self.assertEqual(command.call_args.kwargs["env"], environment)

    @patch.object(package.shutil, "which", return_value=None)
    @patch.object(package.subprocess, "run")
    def test_missing_debugger_does_not_mask_original_failure(self, command, which):
        package.diagnose_viewer_failure(Path("probe"), Path("unused"), {})
        command.assert_not_called()

    @patch.object(package.shutil, "which", return_value="lldb")
    @patch.object(package.subprocess, "run", side_effect=subprocess.TimeoutExpired(["lldb"], 60))
    def test_debugger_timeout_is_best_effort(self, command, which):
        with tempfile.TemporaryDirectory() as directory:
            package.diagnose_viewer_failure(Path("probe"), Path(directory), {})


class PackageTests(unittest.TestCase):
    def create_bundle(self, directory):
        app = Path(directory) / "RSS Guard.app"
        for relative in ("Contents/MacOS/rssguard", "Contents/MacOS/rssguard-article-extractor",
                         "Contents/Frameworks/librssguard.dylib", "Contents/PlugIns/sqldrivers/libqsqlite.dylib"):
            binary = app / relative
            binary.parent.mkdir(parents=True, exist_ok=True)
            binary.write_bytes(b"\xcf\xfa\xed\xfe")
        with (app / "Contents/Info.plist").open("wb") as file:
            plistlib.dump({"CFBundleExecutable": "rssguard", "LSMinimumSystemVersion": "13.0"}, file)
        return app

    @patch.object(package, "inspect_binary", return_value=[])
    @patch.object(package, "run", return_value="Mach-O 64-bit executable arm64")
    def test_complete_bundle_checks_every_required_binary_and_signature(self, command, inspect):
        with tempfile.TemporaryDirectory() as directory:
            app = self.create_bundle(directory)
            package.validate_bundle(app, "arm64", "text", "13.0")
            self.assertEqual(inspect.call_count, 4)
            self.assertEqual(command.call_args.args[0][:4], ["codesign", "--verify", "--deep", "--strict"])

    def test_rejects_missing_sqlite_driver(self):
        with tempfile.TemporaryDirectory() as directory:
            app = self.create_bundle(directory)
            (app / "Contents/PlugIns/sqldrivers/libqsqlite.dylib").unlink()
            with self.assertRaisesRegex(RuntimeError, "SQLite driver"):
                package.validate_bundle(app, "arm64", "text", "13.0")

    def test_web_bundle_requires_renderer_helper(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RuntimeError, "QtWebEngineProcess"):
                package.validate_bundle(self.create_bundle(directory), "arm64", "web", "13.0")

    def test_probe_must_not_be_shipped(self):
        with tempfile.TemporaryDirectory() as directory:
            app = self.create_bundle(directory)
            (app / "Contents/MacOS/rssguard-package-smoke").touch()
            with self.assertRaisesRegex(RuntimeError, "must not be shipped"):
                package.validate_bundle(app, "arm64", "text", "13.0")

    @patch.object(package, "inspect_binary", return_value=[])
    @patch.object(package, "run", return_value="Mach-O 64-bit executable arm64")
    def test_placeholder_sqlite_driver_is_rejected(self, command, inspect):
        with tempfile.TemporaryDirectory() as directory:
            app = self.create_bundle(directory)
            (app / "Contents/PlugIns/sqldrivers/libqsqlite.dylib").write_bytes(b"not a binary")
            with self.assertRaisesRegex(RuntimeError, "not a checked Mach-O"):
                package.validate_bundle(app, "arm64", "text", "13.0")

    def test_selects_all_four_package_names(self):
        for architecture, suffix in package.SUFFIXES.items():
            for variant in ("web", "text"):
                with self.subTest(architecture=architecture, variant=variant), tempfile.TemporaryDirectory() as directory:
                    path = Path(directory) / f"rssguard-5.2.6-{variant}-qt6-{suffix}.dmg"
                    path.touch()
                    self.assertEqual(package.select_package(Path(directory), architecture, variant), path)

    def test_development_package_name(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "rssguard-dev-abcd123-web-qt6-mac64arm.dmg"
            path.touch()
            self.assertEqual(package.select_package(Path(directory), "arm64", "web"), path)

    def test_rejects_wrong_architecture_variant_and_legacy_name(self):
        for suffix in ("text-qt6-mac64arm", "web-qt6-mac64intel", "web-qt6-mac64"):
            with self.subTest(suffix=suffix), tempfile.TemporaryDirectory() as directory:
                (Path(directory) / f"rssguard-5.2.6-{suffix}.dmg").touch()
                with self.assertRaises(RuntimeError):
                    package.select_package(Path(directory), "arm64", "web")

    def test_rejects_missing_and_extra_packages(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with self.assertRaises(RuntimeError):
                package.select_package(root, "arm64", "web")
            for name in ("5.2.5", "5.2.6"):
                (root / f"rssguard-{name}-web-qt6-mac64arm.dmg").touch()
            with self.assertRaises(RuntimeError):
                package.select_package(root, "arm64", "web")

    def test_bundled_and_system_dependencies(self):
        with tempfile.TemporaryDirectory() as directory:
            app = Path(directory) / "RSS Guard.app"
            frameworks = app / "Contents/Frameworks"
            frameworks.mkdir(parents=True)
            (frameworks / "librssguard.dylib").touch()
            binary = app / "Contents/MacOS/rssguard"
            package.check_dependency("@rpath/librssguard.dylib", binary, app)
            package.check_dependency("@executable_path/../Frameworks/librssguard.dylib", binary, app)
            package.check_dependency("/usr/lib/libSystem.B.dylib", binary, app)
            package.check_dependency("/System/Library/Frameworks/AppKit.framework/AppKit", binary, app)

    def test_rejects_missing_external_and_escaping_dependencies(self):
        with tempfile.TemporaryDirectory() as directory:
            app = Path(directory) / "RSS Guard.app"
            app.mkdir()
            (Path(directory) / "external.dylib").touch()
            binary = app / "Contents/MacOS/rssguard"
            for name in ("@rpath/missing.dylib", "/opt/homebrew/lib/libicuuc.dylib",
                         "@loader_path/../../../external.dylib"):
                with self.subTest(name=name), self.assertRaises(RuntimeError):
                    package.check_dependency(name, binary, app)

    @patch.object(package, "run", return_value="")
    def test_failed_validation_detaches_dmg(self, command):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(RuntimeError):
                with package.mounted_package(Path("package.dmg"), Path(directory) / "mount"):
                    pass
        self.assertEqual(command.call_args.args[0][:2], ["hdiutil", "detach"])

    def test_smoke_environment_removes_sdk_fallbacks(self):
        with patch.dict(os.environ, {"QT_PLUGIN_PATH": "/sdk/plugins", "DYLD_LIBRARY_PATH": "/sdk/lib",
                                     "QTWEBENGINEPROCESS_PATH": "/sdk/helper", "PATH": "normal-path"}):
            environment = package.smoke_environment()
        self.assertNotIn("QT_PLUGIN_PATH", environment)
        self.assertNotIn("DYLD_LIBRARY_PATH", environment)
        self.assertNotIn("QTWEBENGINEPROCESS_PATH", environment)
        self.assertEqual(environment["PATH"], "normal-path")


if __name__ == "__main__":
    unittest.main()
