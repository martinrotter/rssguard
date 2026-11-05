"""Validate payload generation and exercise the real NSIS template in an isolated user scope."""

import ctypes
from contextlib import contextmanager
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
import uuid


ROOT = Path(__file__).resolve().parents[4]
SCRIPT = Path(__file__).resolve().parents[1] / "prepare-windows-installer.py"
spec = importlib.util.spec_from_file_location("windows_installer", SCRIPT)
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)
NSIS = ROOT / "resources/scripts/nsis/makensis.exe"
OWNERSHIP = ".rssguard-install.ini"


@contextmanager
def locked_file(path):
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateFileW.argtypes = (ctypes.c_wchar_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p,
                                  ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p)
    kernel.CreateFileW.restype = ctypes.c_void_p
    kernel.CloseHandle.argtypes = (ctypes.c_void_p,)
    handle = kernel.CreateFileW(str(path), 0x80000000, 1, None, 3, 0, None)
    if handle == ctypes.c_void_p(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        yield
    finally:
        kernel.CloseHandle(handle)


def definitions(stage, name="RSS Guard fixture", executable="rssguard.exe", version="5.3.0", xmpp=False):
    values = {
        "APP_VERSION": version, "APP_VERSION_MAJOR": version.split('.')[0], "APP_NAME": name,
        "APP_AUTHOR": "Installer regression tests", "APP_URL": "https://example.invalid",
        "APP_DOCUMENTATION": "https://example.invalid/docs", "EXE_NAME": executable,
        "LICENSE_FILE": str(ROOT / "resources/text/COPYING_GNU_GPL"),
        "MUI_ICON": str(ROOT / "resources/graphics/rssguard.ico"),
        "MUI_UNICON": str(ROOT / "resources/graphics/rssguard.ico"),
        "BINARY_TREE": str(stage), "PLUGIN_XMPP_ENABLED": str(int(xmpp)),
    }
    for plugin in (*package.PLUGINS, "xmpp"):
        values["PLUGIN_" + plugin.upper()] = str(stage / "plugins" / f"rssguard-{plugin}.dll")
    return '\n'.join('!define ' + key + ' ' + package.quoted(value) for key, value in values.items()) + '\n'


def make_payload(stage, executable="rssguard.exe", xmpp=False):
    names = [executable, "rssguard.dll", "rssguard-article-extractor.exe", "platforms/qwindows.dll",
             "resources/žluťoučký.txt", "rssguard-extra$helper.exe", "rssguard.pdb", "plugins/rssguard-feedly.pdb",
             "zz-recovery-test.dll"]  # Removed after the main binaries: exercises genuinely partial cleanup.
    names += [f"plugins/rssguard-{plugin}.dll" for plugin in (*package.PLUGINS, *(("xmpp",) if xmpp else ()))]
    for name in names:
        path = stage / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"fixture")


class PayloadTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="rssguard-nsis-payload-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.assertEqual(self.root.parent, Path(tempfile.gettempdir()).resolve())
        self.stage = self.root / "app"
        make_payload(self.stage)
        self.defs = self.root / "NSIS.definitions.nsh"
        self.defs.write_text(definitions(self.stage), encoding="utf-8")
        self.output = self.root / "NSIS.payload.nsh"

    def test_exact_payload_preserves_unknown_files(self):
        (self.stage / "rssguard-new-helper.exe").write_bytes(b"helper")
        package.generate(self.stage, self.defs, self.output)
        result = self.output.read_text(encoding="utf-8")
        self.assertIn('rssguard-new-helper.exe', result)
        self.assertNotIn('.pdb', result)
        self.assertNotIn('RMDir /r', result)
        self.assertNotIn('Delete "$INSTDIR\\*"', result)
        self.assertIn('Delete "$INSTDIR\\rssguard-article-extractor"', result)

    def test_missing_required_files(self):
        for name in ("rssguard.exe", "rssguard.dll", "rssguard-article-extractor.exe",
                     "platforms/qwindows.dll", "plugins/rssguard-standard.dll"):
            with self.subTest(name=name):
                path = self.stage / name
                contents = path.read_bytes()
                path.unlink()
                with self.assertRaisesRegex(ValueError, "Missing package files"):
                    package.generate(self.stage, self.defs, self.output)
                path.write_bytes(contents)

    def test_reject_user_data(self):
        (self.stage / "Data5").mkdir()
        with self.assertRaisesRegex(ValueError, "User data"):
            package.generate(self.stage, self.defs, self.output)

    def test_reject_installer_recovery_files(self):
        for name in (OWNERSHIP, "Uninstall.exe"):
            with self.subTest(name=name):
                path = self.stage / name
                path.write_bytes(b"must not overwrite runtime recovery files")
                try:
                    with self.assertRaisesRegex(ValueError, "Installer recovery files"):
                        package.generate(self.stage, self.defs, self.output)
                finally:
                    path.unlink()

    def test_disabled_xmpp_is_not_silently_packaged(self):
        (self.stage / "plugins/rssguard-xmpp.dll").write_bytes(b"fixture")
        with self.assertRaisesRegex(ValueError, "Unexpected service plugin"):
            package.generate(self.stage, self.defs, self.output)
        self.defs.write_text(definitions(self.stage, xmpp=True), encoding="utf-8")
        package.generate(self.stage, self.defs, self.output)

    def test_missing_enabled_xmpp_is_rejected(self):
        self.defs.write_text(definitions(self.stage, xmpp=True), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "rssguard-xmpp.dll"):
            package.generate(self.stage, self.defs, self.output)

    def test_dollar_in_filename_is_escaped(self):
        (self.stage / "helper$special.exe").write_bytes(b"fixture")
        package.generate(self.stage, self.defs, self.output)
        self.assertIn('helper$$special.exe', self.output.read_text(encoding="utf-8"))

    @unittest.skipIf(os.name == "nt", "Creating Windows symlinks may require developer mode")
    def test_links_and_case_collisions_are_rejected(self):
        link = self.stage / "linked"
        link.symlink_to(self.root, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, "link or reparse point"):
            package.generate(self.stage, self.defs, self.output)
        link.unlink()
        (self.stage / "RSSGUARD.EXE").write_bytes(b"fixture")
        with self.assertRaisesRegex(ValueError, "Case-colliding"):
            package.generate(self.stage, self.defs, self.output)


@unittest.skipUnless(shutil.which("cmake") and shutil.which("ninja"), "CMake and Ninja are required")
class BuildRulesTests(unittest.TestCase):
    def test_template_edits_trigger_incremental_reconfiguration(self):
        with tempfile.TemporaryDirectory(prefix="rssguard-nsis-cmake-") as directory:
            root = Path(directory).resolve()
            self.assertEqual(root.parent, Path(tempfile.gettempdir()).resolve())
            source, build = root / "source", root / "build"
            template = source / "resources/nsis/NSIS.template.in"
            template.parent.mkdir(parents=True)
            shutil.copyfile(ROOT / "resources/nsis/NSIS.template.in", template)
            # Exercise the actual production rule, without configuring the Qt application.
            cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
            rule = next(line.strip() for line in cmake.splitlines()
                        if "configure_file(resources/nsis/NSIS.template.in" in line)
            major = next(line.strip() for line in cmake.splitlines() if "MATCH" in line and "APP_VERSION_MAJOR" in line)
            (source / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.21)\nproject(nsis_fixture NONE)\n' + rule +
                '\nset(APP_VERSION "10.2.0")\n' + major +
                '\nfile(WRITE "${CMAKE_BINARY_DIR}/major.txt" "${APP_VERSION_MAJOR}")\n', encoding="utf-8")
            for command in (["cmake", "-G", "Ninja", "-S", str(source), "-B", str(build)],
                            ["cmake", "--build", str(build)]):
                result = subprocess.run(command, capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            copied = build / "NSIS.template.in"
            self.assertEqual(copied.read_bytes(), template.read_bytes())
            self.assertEqual((build / "major.txt").read_text(), "10")
            with template.open("a", encoding="utf-8") as output:
                output.write("\n; Incremental packaging regression\n")
            result = subprocess.run(["cmake", "--build", str(build)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(copied.read_bytes(), template.read_bytes())


@unittest.skipUnless(os.name == "nt" and NSIS.exists(), "Windows and bundled NSIS are required")
class InstallerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import winreg
        cls.winreg = winreg
        cls.temporary = tempfile.TemporaryDirectory(prefix="rssguard-nsis-runtime-")
        cls.root = Path(cls.temporary.name).resolve()
        assert cls.root.parent == Path(tempfile.gettempdir()).resolve()
        cls.identifier = uuid.uuid4().hex
        cls.name = "RSS Guard installer test " + cls.identifier
        cls.executable = "rssguard-fixture-" + cls.identifier + ".exe"
        cls.key = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + cls.name + " 5"
        cls.desktop = cls.root / "shortcuts/desktop"
        cls.programs = cls.root / "shortcuts/programs"
        cls.desktop.mkdir(parents=True)
        cls.programs.mkdir(parents=True)
        # Only the privilege/scope boundary is changed. All installer decisions use the real template.
        cls.template = (ROOT / "resources/nsis/NSIS.template.in").read_text(encoding="utf-8")
        cls.template = cls.template.replace("RequestExecutionLevel admin", "RequestExecutionLevel user")
        cls.template = cls.template.replace("HKLM", "HKCU").replace("SetShellVarContext all", "SetShellVarContext current")
        cls.template = cls.template.replace("$DESKTOP", str(cls.desktop)).replace("$SMPROGRAMS", str(cls.programs))
        cls.failure_log = cls.root / "last-error.txt"
        cls.template = cls.template.replace('  SetErrorLevel 2',
            '  FileOpen $9 ' + package.quoted(cls.failure_log) + ' w\n'
            '  FileWrite $9 "${Message}"\n  FileClose $9\n  SetErrorLevel 2')
        cls.old = cls.build("5.2.0", legacy=True)
        cls.new = cls.build("5.3.0", xmpp=True)
        cls.minimal = cls.build("5.3.0", disabled=("PluginFeedly", "PluginGmail", "PluginGreader",
                                                  "PluginNextcloud", "PluginTtrss"))
        cls.failing = cls.helper("failing.exe", "SetErrorLevel 7\nQuit")
        cls.noop = cls.helper("noop.exe", "SetErrorLevel 0\nQuit")

    @classmethod
    def compile(cls, script, output, cwd, strict=False):
        command = [str(NSIS), "/NOCONFIG", "/NOCD", "/V2", "/XOutFile " + str(output), "-"]
        if strict:
            command.insert(1, "/WX")
        result = subprocess.run(command, input=script, cwd=cwd, capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)
        return output

    @classmethod
    def helper(cls, filename, body, version="5.3.0"):
        output = cls.root / filename
        script = (f'Unicode True\nName "Fixture"\nRequestExecutionLevel user\nSilentInstall silent\n'
                  f'VIProductVersion "{version}.0"\nVIFileVersion "{version}.0"\n'
                  f'VIAddVersionKey "FileVersion" "{version}"\n'
                  f'Section\n{body}\nSectionEnd\n')
        return cls.compile(script, output, cls.root)

    @classmethod
    def build(cls, version, xmpp=False, disabled=(), legacy=False):
        directory = cls.root / (version + ("-minimal" if disabled else ""))
        directory.mkdir()
        stage = directory / "app"
        make_payload(stage, cls.executable, xmpp)
        shutil.copyfile(cls.helper("app-" + version + ".exe", "Sleep 60000", version), stage / cls.executable)
        defs = directory / "NSIS.definitions.nsh"
        defs.write_text(definitions(stage, cls.name, cls.executable, version, xmpp), encoding="utf-8")
        package.generate(stage, defs, directory / "NSIS.payload.nsh")
        if legacy:
            template = (Path(__file__).parent / "fixtures/NSIS.previous.nsi").read_text(encoding="utf-8")
            template = template.replace("RequestExecutionLevel admin", "RequestExecutionLevel user")
            template = template.replace("HKLM", "HKCU")
            template = template.replace("$DESKTOP", str(cls.desktop)).replace("$SMPROGRAMS", str(cls.programs))
            # The legacy filename exclusion otherwise omits our uniquely named dummy executable.
            template = template.replace('  File "${BINARY_TREE}\\rssguard-article-extractor.exe"',
                '  File "${BINARY_TREE}\\rssguard-article-extractor.exe"\n  File "${BINARY_TREE}\\${EXE_NAME}"')
            template = template.replace('ExecShell "open" "https://rssguard.readthedocs.io/en/stable"',
                                        'DetailPrint "Documentation suppressed in fixture"')
        else:
            template = cls.template.replace("Function .onInit\n", "Function .onInit\n" +
                                            ''.join("  SectionSetFlags ${" + name + "} 0\n" for name in disabled))
        return cls.compile(template, directory / "installer.exe", directory, strict=True)

    def test_production_template_compiles_without_scope_substitutions(self):
        template = (ROOT / "resources/nsis/NSIS.template.in").read_text(encoding="utf-8")
        self.compile(template, self.root / "production.exe", self.root / "5.3.0", strict=True)

    @unittest.skipUnless(shutil.which("pwsh") and shutil.which("git") and
                         (ROOT / "resources/scripts/7za/7za.exe").exists(), "PowerShell, Git and bundled 7za are required")
    def test_actual_packaging_script_validates_before_writing_archives(self):
        workspace = self.root / "package smoke"
        scripts = workspace / "resources/scripts"
        actions = scripts / "github-actions"
        actions.mkdir(parents=True)
        for name in ("package-windows.ps1", "prepare-windows-installer.py"):
            shutil.copyfile(SCRIPT.parent / name, actions / name)
        shutil.copytree(NSIS.parent, scripts / "nsis", ignore=shutil.ignore_patterns(".git"))
        (scripts / "7za").mkdir()
        shutil.copyfile(ROOT / "resources/scripts/7za/7za.exe", scripts / "7za/7za.exe")
        build = workspace / "rssguard-build"
        stage = build / "app"
        make_payload(stage, self.executable, xmpp=True)
        (build / "NSIS.definitions.nsh").write_text(
            definitions(stage, self.name, self.executable, xmpp=True), encoding="utf-8")
        shutil.copyfile(ROOT / "resources/nsis/NSIS.template.in", build / "NSIS.template.in")
        # A disposable repository supplies the unchanged packaging script's revision/tag inputs.
        environment = {name: value for name, value in os.environ.items() if not name.startswith("GIT_")}
        environment["GIT_CONFIG_GLOBAL"] = os.devnull
        environment["GIT_CONFIG_SYSTEM"] = os.devnull
        for args in (("init", "--quiet"),
                     ("-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid",
                      "-c", "commit.gpgsign=false", "-c", "core.hooksPath=" + str(self.root / "no-hooks"),
                      "commit", "--quiet", "--allow-empty", "-m", "Fixture"),
                     ("tag", "5.3.0")):
            result = subprocess.run(["git", *args], cwd=workspace, env=environment, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        binary_dir = workspace / "bin"
        binary_dir.mkdir()
        (binary_dir / "python.cmd").write_text('@echo off\n"' + sys.executable + '" %*\n', encoding="utf-8")
        environment["PATH"] = str(binary_dir) + os.pathsep + environment["PATH"]
        environment["GITHUB_REF"] = "refs/heads/fixture"
        command = ["pwsh", "-NoLogo", "-NoProfile", "-File", str(actions / "package-windows.ps1"),
                   "windows-2022", "OFF", "OFF"]
        extractor = stage / "rssguard-article-extractor.exe"
        backup = extractor.with_suffix(".missing")
        assert extractor.resolve().is_relative_to(workspace.resolve())
        assert backup.resolve().is_relative_to(workspace.resolve())
        extractor.rename(backup)
        try:
            failed = subprocess.run(command, cwd=workspace, env=environment, capture_output=True,
                                    text=True, errors="replace", timeout=60)
            self.assertNotEqual(failed.returncode, 0)
            self.assertEqual(list(build.glob("*.7z")) + list(build.glob("*.exe")), [])
        finally:
            backup.rename(extractor)
        result = subprocess.run(command, cwd=workspace, env=environment, capture_output=True,
                                text=True, errors="replace", timeout=60)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(len(list(build.glob("*.exe"))), 1)
        archives = list(build.glob("*.7z"))
        self.assertEqual(len(archives), 1)
        listing = subprocess.run([str(scripts / "7za/7za.exe"), "l", str(archives[0])],
                                 capture_output=True, text=True, errors="replace", timeout=30)
        self.assertEqual(listing.returncode, 0)
        self.assertIn("rssguard.pdb", listing.stdout)

    def setUp(self):
        self.target = self.root / ("install ü with spaces " + uuid.uuid4().hex)
        self.clear_registration()

    @classmethod
    def clear_registration(cls):
        try:
            cls.winreg.DeleteKeyEx(cls.winreg.HKEY_CURRENT_USER, cls.key, cls.winreg.KEY_WOW64_32KEY)
        except FileNotFoundError:
            pass

    def registration(self, name):
        with self.winreg.OpenKey(self.winreg.HKEY_CURRENT_USER, self.key, 0,
                                 self.winreg.KEY_READ | self.winreg.KEY_WOW64_32KEY) as key:
            return self.winreg.QueryValueEx(key, name)[0]

    def set_registration(self, name, value):
        with self.winreg.CreateKeyEx(self.winreg.HKEY_CURRENT_USER, self.key, 0,
                                     self.winreg.KEY_SET_VALUE | self.winreg.KEY_WOW64_32KEY) as key:
            self.winreg.SetValueEx(key, name, 0, self.winreg.REG_SZ, str(value))

    def install(self, installer=None, target=None, expected=0):
        command = '"' + str(installer or self.new) + '" /S /D=' + str(target or self.target)
        result = subprocess.run(command, timeout=30)
        self.assertIn(result.returncode, (expected,) if isinstance(expected, int) else expected,
                         self.failure_log.read_text(errors="replace") if self.failure_log.exists() else "")

    def uninstall(self, target=None, expected=0):
        target = target or self.target
        # Explicit copying and _?= make the test wait for the actual uninstall outcome too.
        copied = self.root / ("uninstall-" + uuid.uuid4().hex + ".exe")
        shutil.copyfile(target / "Uninstall.exe", copied)
        result = subprocess.run('"' + str(copied) + '" /S _?=' + str(target), timeout=30)
        self.assertEqual(result.returncode, expected,
                         self.failure_log.read_text(errors="replace") if self.failure_log.exists() else "")

    def test_fresh_install_and_owned_uninstall(self):
        self.install()
        self.assertEqual(self.registration("DisplayVersion"), "5.3.0")
        self.assertEqual(self.registration("NoRepair"), 1)
        self.assertGreater(self.registration("EstimatedSize"), 0)
        self.assertTrue((self.target / "plugins/rssguard-xmpp.dll").exists())
        self.assertTrue((self.target / "rssguard-extra$helper.exe").exists())
        self.assertTrue((self.target / "resources/žluťoučký.txt").exists())
        self.assertFalse((self.target / "rssguard.pdb").exists())
        retained = ["data4/config.ini", "data5/database.db", "profile/notes.txt", "notes.txt",
                    "plugins/my-file.txt", "platforms/my-file.txt"]
        for relative in retained:
            path = self.target / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("keep", encoding="utf-8")
        self.uninstall()
        for relative in retained:
            self.assertEqual((self.target / relative).read_text(encoding="utf-8"), "keep")
        self.assertFalse((self.target / self.executable).exists())
        self.assertFalse((self.target / OWNERSHIP).exists())
        with self.assertRaises(FileNotFoundError):
            self.registration("InstallLocation")

    def test_upgrade_uses_old_uninstaller_and_preserves_data(self):
        self.install(self.old)
        self.assertFalse((self.target / OWNERSHIP).exists(), "Legacy fixture must use the actual previous policy")
        path = self.target / "data5/database.db"
        path.parent.mkdir()
        path.write_text("keep", encoding="utf-8")
        self.install()
        self.assertEqual(path.read_text(encoding="utf-8"), "keep")
        self.assertEqual(self.registration("DisplayVersion"), "5.3.0")
        self.uninstall()

    def test_legacy_upgrade_failure_can_be_retried(self):
        self.install(self.old)
        data = self.target / "data5/database.db"
        data.parent.mkdir()
        data.write_text("keep", encoding="utf-8")
        with locked_file(self.target / "zz-recovery-test.dll"):
            self.install(expected=(1, 2))
            self.assertTrue((self.target / OWNERSHIP).exists())
            self.assertTrue((self.target / "Uninstall.exe").exists())
            with self.assertRaises(FileNotFoundError):
                self.registration("InstallLocation")
        personal = self.target / "profile/notes.txt"
        personal.parent.mkdir()
        personal.write_text("keep", encoding="utf-8")
        self.install()
        self.assertEqual(self.registration("DisplayVersion"), "5.3.0")
        self.assertTrue((self.target / "plugins/rssguard-standard.dll").exists())
        self.assertEqual(data.read_text(), "keep")
        self.assertEqual(personal.read_text(), "keep")
        self.uninstall()

    def test_late_fresh_install_failure_can_be_retried(self):
        shortcut = self.desktop / (self.name + " 5.lnk")
        shortcut.unlink(missing_ok=True)
        shortcut.mkdir()  # Real shortcut failure after all application files have been extracted.
        try:
            self.install(expected=2)
            self.assertIn("desktop shortcut", self.failure_log.read_text())
            self.assertTrue((self.target / OWNERSHIP).exists())
            self.assertTrue((self.target / "Uninstall.exe").exists())
            with self.assertRaises(FileNotFoundError):
                self.registration("InstallLocation")
        finally:
            shortcut.rmdir()
        data = self.target / "data5/database.db"
        data.parent.mkdir()
        data.write_text("keep", encoding="utf-8")
        self.install()
        self.assertEqual(self.registration("DisplayVersion"), "5.3.0")
        self.assertEqual(data.read_text(), "keep")
        self.uninstall()

    def test_owned_partial_images_can_be_repaired(self):
        self.install()
        (self.target / self.executable).write_bytes(b"interrupted executable write")
        (self.target / "Uninstall.exe").write_bytes(b"interrupted uninstaller write")
        self.install()
        self.assertEqual(self.registration("DisplayVersion"), "5.3.0")
        self.assertTrue((self.target / "Uninstall.exe").read_bytes().startswith(b"MZ"))
        self.uninstall()

    def test_optional_plugins_can_be_omitted(self):
        self.install(self.minimal)
        self.assertEqual(sorted(path.name for path in (self.target / "plugins").iterdir()),
                         ["rssguard-standard.dll"])
        self.uninstall()

    def test_uninstaller_failures_stop_upgrade(self):
        for source in (self.failing, self.noop):
            with self.subTest(source=source.name):
                self.clear_registration()
                self.target = self.root / uuid.uuid4().hex
                self.install(self.old)
                shutil.copyfile(source, self.target / "Uninstall.exe")
                original = (self.target / self.executable).read_bytes()
                self.install(expected=2)
                self.assertEqual((self.target / self.executable).read_bytes(), original)
                self.assertEqual(self.registration("DisplayVersion"), "5.2.0")

    def test_bad_uninstaller_cannot_be_launched(self):
        self.install(self.old)
        (self.target / "Uninstall.exe").write_text("not executable", encoding="utf-8")
        self.install(expected=2)
        self.assertEqual(self.registration("DisplayVersion"), "5.2.0")

    def test_unsafe_legacy_versions_are_not_executed(self):
        self.install(self.old)
        for version in ("5.0.0", "5.0.1"):
            self.set_registration("DisplayVersion", version)
            self.install(expected=2)
            self.assertTrue((self.target / self.executable).exists())
        self.set_registration("DisplayVersion", "5.2.0")
        shutil.copyfile(self.helper("unsafe-app.exe", "Quit", "5.0.1"), self.target / self.executable)
        self.install(expected=2)
        self.assertIn("5.0.0 and 5.0.1", self.failure_log.read_text())

    def test_shared_and_dangerous_roots_are_rejected(self):
        self.target.mkdir()
        sentinel = self.target / "unrelated.txt"
        sentinel.write_text("keep", encoding="utf-8")
        self.install(expected=2)
        self.assertEqual(sentinel.read_text(encoding="utf-8"), "keep")
        for target in (Path(self.root.anchor), Path(tempfile.gettempdir()), Path(os.environ["USERPROFILE"]),
                       str(self.desktop) + "\\", tempfile.gettempdir() + "\\", r"\\?\C:\Windows",
                       r"C:\PROGRA~1"):
            with self.subTest(target=str(target)):
                self.install(target=target, expected=2)
                with self.assertRaises(FileNotFoundError):
                    self.registration("InstallLocation")

    def test_retained_data_directory_can_be_reused(self):
        path = self.target / "Data5/database.db"
        path.parent.mkdir(parents=True)
        path.write_text("keep", encoding="utf-8")
        self.install()
        self.assertEqual(path.read_text(encoding="utf-8"), "keep")
        self.uninstall()

    def test_running_application_blocks_install_and_uninstall(self):
        self.install()
        process = subprocess.Popen([str(self.target / self.executable), "/S"])
        try:
            time.sleep(0.2)
            self.assertIsNone(process.poll())
            self.install(expected=2)
            self.uninstall(expected=2)
            self.assertEqual(self.registration("DisplayVersion"), "5.3.0")
        finally:
            process.terminate()
            process.wait(timeout=10)
        self.uninstall()

    def test_partial_removal_keeps_registration_and_uninstaller(self):
        self.install()
        with locked_file(self.target / "rssguard.dll"):
            self.install(expected=2)
            self.uninstall(expected=2)
            self.assertTrue((self.target / "Uninstall.exe").exists())
            self.assertEqual(self.registration("InstallLocation"), str(self.target))
        self.uninstall()

    def test_ownership_removal_failure_can_be_retried(self):
        self.install()
        with locked_file(self.target / OWNERSHIP):
            self.uninstall(expected=2)
            self.assertFalse((self.target / self.executable).exists())
            self.assertTrue((self.target / "Uninstall.exe").exists())
            self.assertTrue((self.target / OWNERSHIP).exists())
            self.assertEqual(self.registration("InstallLocation"), str(self.target))
        self.uninstall()
        self.assertFalse((self.target / OWNERSHIP).exists())

    def test_failed_repair_does_not_register_a_new_version(self):
        self.install(self.old)
        (self.target / "Uninstall.exe").unlink()  # Repair an otherwise recognized installation.
        with locked_file(self.target / "rssguard.dll"):
            self.install(expected=(1, 2))  # NSIS can abort extraction before the script's error handler.
            self.assertEqual(self.registration("DisplayVersion"), "5.2.0")
        self.install()
        self.uninstall()

    def test_moved_uninstaller_does_not_remove_files(self):
        self.install()
        moved = self.root / ("moved-" + uuid.uuid4().hex)
        moved.mkdir()
        shutil.copyfile(self.target / "Uninstall.exe", moved / "Uninstall.exe")
        self.uninstall(moved, expected=2)
        self.assertTrue((self.target / self.executable).exists())
        self.assertEqual(self.registration("InstallLocation"), str(self.target))
        self.uninstall()

    def test_junction_is_not_followed_during_removal(self):
        self.install()
        outside = self.root / ("outside-" + uuid.uuid4().hex)
        outside.mkdir()
        sentinel = outside / "qwindows.dll"
        sentinel.write_text("keep", encoding="utf-8")
        link = self.target / "platforms"
        (link / "qwindows.dll").unlink()
        link.rmdir()
        command = 'mklink /J "' + str(link) + '" "' + str(outside) + '"'
        result = subprocess.run("cmd.exe /d /c " + command, capture_output=True, text=True,
                                errors="replace", creationflags=subprocess.CREATE_NO_WINDOW)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        try:
            self.uninstall(expected=2)
            self.assertEqual(sentinel.read_text(encoding="utf-8"), "keep")
            self.assertTrue((self.target / self.executable).exists())
        finally:
            self.assertTrue(link.is_junction())
            link.rmdir()  # Removes only the junction, never its target.
        link.mkdir()
        (link / "qwindows.dll").write_bytes(b"fixture")
        self.uninstall()

    def test_other_installation_keeps_its_registration_and_shortcuts(self):
        self.install()
        first = self.target
        self.target = self.root / uuid.uuid4().hex
        self.install()
        shortcut = self.desktop / (self.name + " 5.lnk")
        self.assertTrue(shortcut.exists())
        self.uninstall(first)
        self.assertEqual(self.registration("InstallLocation"), str(self.target))
        self.assertTrue(shortcut.exists())
        self.uninstall()

    def test_superseded_partial_removal_can_be_retried(self):
        self.install()
        first = self.target
        data = first / "data5/database.db"
        data.parent.mkdir()
        data.write_text("keep", encoding="utf-8")
        self.target = self.root / uuid.uuid4().hex
        self.install()
        shortcut = self.desktop / (self.name + " 5.lnk")
        contents = shortcut.read_bytes()
        with locked_file(first / "zz-recovery-test.dll"):
            self.uninstall(first, expected=2)
            self.assertFalse((first / self.executable).exists())
            self.assertFalse((first / "rssguard.dll").exists())
            self.assertTrue((first / "Uninstall.exe").exists())
            self.assertTrue((first / OWNERSHIP).exists())
        self.uninstall(first)
        self.assertFalse((first / OWNERSHIP).exists())
        self.assertFalse((first / "zz-recovery-test.dll").exists())
        self.assertEqual(data.read_text(), "keep")
        self.assertEqual(self.registration("InstallLocation"), str(self.target))
        self.assertEqual(shortcut.read_bytes(), contents)
        self.assertTrue((self.target / OWNERSHIP).exists())
        self.uninstall()

    def test_copied_ownership_record_does_not_authorize_another_folder(self):
        self.install()
        unrelated = self.root / ("unrelated-" + uuid.uuid4().hex)
        unrelated.mkdir()
        shutil.copyfile(self.target / OWNERSHIP, unrelated / OWNERSHIP)
        sentinel = unrelated / "notes.txt"
        sentinel.write_text("keep", encoding="utf-8")
        self.install(target=unrelated, expected=2)
        shutil.copyfile(self.target / "Uninstall.exe", unrelated / "Uninstall.exe")
        self.uninstall(unrelated, expected=2)
        self.assertEqual(sentinel.read_text(), "keep")
        self.assertEqual(self.registration("InstallLocation"), str(self.target))
        self.uninstall()

    def test_local_recovery_ignores_another_copys_version(self):
        self.install()
        first = self.target
        self.target = self.root / uuid.uuid4().hex
        self.install()
        second = self.target
        self.set_registration("DisplayVersion", "5.0.1")  # Belongs to the other directory, not this repair.
        (first / self.executable).unlink()
        self.install(target=first)
        self.assertEqual(self.registration("InstallLocation"), str(first))
        self.assertEqual(self.registration("DisplayVersion"), "5.3.0")
        self.assertTrue((second / OWNERSHIP).exists())
        self.uninstall(second)
        self.assertEqual(self.registration("InstallLocation"), str(first))
        self.uninstall(first)

    @classmethod
    def tearDownClass(cls):
        cls.clear_registration()
        cls.temporary.cleanup()


if __name__ == "__main__":
    unittest.main()
