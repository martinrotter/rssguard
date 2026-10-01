#!/usr/bin/env python3
"""Check CPU/OS compatibility and exercise the contents of the finished macOS DMG."""

import argparse
import contextlib
import json
import os
from pathlib import Path
import platform
import plistlib
import re
import shutil
import signal
import subprocess
import tempfile


SUFFIXES = {"x86_64": "mac64intel", "arm64": "mac64arm"}
DYLIB_COMMANDS = {
    "LC_LOAD_DYLIB", "LC_LOAD_WEAK_DYLIB", "LC_REEXPORT_DYLIB",
    "LC_LAZY_LOAD_DYLIB", "LC_LOAD_UPWARD_DYLIB",
}
MACHO_MAGIC = {
    b"\xfe\xed\xfa\xce", b"\xce\xfa\xed\xfe", b"\xfe\xed\xfa\xcf", b"\xcf\xfa\xed\xfe",
    b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca", b"\xca\xfe\xba\xbf", b"\xbf\xba\xfe\xca",
}


def run(command, **kwargs):
    result = subprocess.run(command, text=True, capture_output=True, timeout=120, **kwargs)
    if result.returncode:
        status = f"exit code {result.returncode}"
        if result.returncode < 0:
            try:
                status = f"signal {signal.Signals(-result.returncode).name} ({result.returncode})"
            except ValueError:
                status = f"signal {-result.returncode}"
        raise RuntimeError(f"Command failed ({status}): {command}\n{result.stdout}\n{result.stderr}")
    return result.stdout


def version_tuple(value):
    if not re.fullmatch(r"\d+(?:\.\d+){0,2}", value):
        raise ValueError(f"Invalid macOS version: {value}")
    parts = tuple(int(part) for part in value.split("."))
    return parts + (0,) * (3 - len(parts))


def load_commands(output):
    return re.split(r"(?=^Load command \d+\s*$)", output, flags=re.MULTILINE)


def minimum_versions(output):
    versions = []
    for block in load_commands(output):
        if re.search(r"^\s*cmd LC_BUILD_VERSION\s*$", block, re.MULTILINE):
            if not re.search(r"^\s*platform (?:1|macos)\s*$", block, re.MULTILINE | re.IGNORECASE):
                raise RuntimeError("A packaged binary targets a platform other than macOS.")
            field = "minos"
        elif re.search(r"^\s*cmd LC_VERSION_MIN_MACOSX\s*$", block, re.MULTILINE):
            field = "version"
        else:
            continue
        match = re.search(rf"^\s*{field}\s+(\d+(?:\.\d+){{0,2}})\s*$", block, re.MULTILINE)
        if not match:
            raise RuntimeError("Missing deployment version in a Mach-O load command.")
        versions.append(match[1])
    if not versions:
        raise RuntimeError("No macOS deployment version found in the binary.")
    return versions


def dependencies(output):
    result = []
    for block in load_commands(output):
        command = re.search(r"^\s*cmd (LC_\w+)\s*$", block, re.MULTILINE)
        if command and command[1] in DYLIB_COMMANDS:
            name = re.search(r"^\s*name (.+) \(offset \d+\)\s*$", block, re.MULTILINE)
            if not name:
                raise RuntimeError("Missing library name in a Mach-O load command.")
            result.append(name[1])
    return result


def inspect_binary(path, architecture, minimum_macos):
    slices = run(["lipo", "-archs", str(path)]).split()
    selected = architecture
    # Qt can provide its Intel WebEngine slice as the Haswell sub-architecture.
    if selected == "x86_64" and selected not in slices and "x86_64h" in slices:
        selected = "x86_64h"
    if selected not in slices:
        raise RuntimeError(f"{path}: expected {architecture}, found {slices}")
    commands = run(["otool", "-arch", selected, "-l", str(path)])
    for minimum in minimum_versions(commands):
        if version_tuple(minimum) > version_tuple(minimum_macos):
            raise RuntimeError(f"{path}: requires macOS {minimum}, above {minimum_macos}")
    return dependencies(commands)


def check_dependency(name, binary, app):
    if name.startswith(("/System/Library/", "/usr/lib/")):
        return
    if name.startswith("@rpath/"):
        relative = name.removeprefix("@rpath/")
        candidates = [app / "Contents/Frameworks" / relative, app / "Contents/MacOS" / relative]
    elif name.startswith("@loader_path/"):
        candidates = [binary.parent / name.removeprefix("@loader_path/")]
    elif name.startswith("@executable_path/"):
        # A nested helper has its own executable directory; shared libraries use the main one.
        executable_directory = binary.parent if binary.parent.name == "MacOS" else app / "Contents/MacOS"
        candidates = [executable_directory / name.removeprefix("@executable_path/")]
    else:
        raise RuntimeError(f"{binary}: external or unsupported dependency {name}")
    for candidate in candidates:
        resolved = candidate.resolve()
        if resolved.is_relative_to(app.resolve()) and resolved.is_file():
            return
    raise RuntimeError(f"{binary}: dependency is missing from the app bundle: {name}")


def select_package(directory, architecture, variant):
    packages = list(directory.glob("rssguard-*.dmg"))
    expected = f"-{variant}-qt6-{SUFFIXES[architecture]}.dmg"
    if len(packages) != 1 or not packages[0].name.endswith(expected):
        raise RuntimeError(f"Expected exactly one {expected} package, found {[p.name for p in packages]}")
    return packages[0]


def validate_bundle(app, architecture, variant, minimum_macos):
    with (app / "Contents/Info.plist").open("rb") as file:
        metadata = plistlib.load(file)
    if version_tuple(metadata.get("LSMinimumSystemVersion", "")) != version_tuple(minimum_macos):
        raise RuntimeError("Info.plist does not declare the expected minimum macOS version.")
    if metadata.get("CFBundleExecutable") != "rssguard":
        raise RuntimeError("Unexpected app executable in Info.plist.")
    required = [
        app / "Contents/MacOS/rssguard",
        app / "Contents/MacOS/rssguard-article-extractor",
        app / "Contents/Frameworks/librssguard.dylib",
    ]
    if not all(path.is_file() for path in required):
        raise RuntimeError("The app bundle is missing an executable or librssguard.")
    if (app / "Contents/MacOS/rssguard-package-smoke").exists():
        raise RuntimeError("The CI smoke-test executable must not be shipped.")
    sqlite_drivers = list(app.rglob("libqsqlite.dylib"))
    if not sqlite_drivers:
        raise RuntimeError("The app bundle is missing its SQLite driver.")
    required.extend(sqlite_drivers)
    if variant == "web":
        helpers = list(app.rglob("QtWebEngineProcess"))
        if not helpers:
            raise RuntimeError("The web package is missing QtWebEngineProcess.")
        required.extend(helpers)

    checked = set()
    for path in app.rglob("*"):
        if not path.is_file() or path.is_symlink():
            continue
        resolved = path.resolve()
        with path.open("rb") as file:
            magic = file.read(4)
        if resolved in checked or magic not in MACHO_MAGIC or "Mach-O" not in run(["file", "-b", str(path)]):
            continue
        checked.add(resolved)
        for name in inspect_binary(path, architecture, minimum_macos):
            check_dependency(name, path, app)
    if not checked:
        raise RuntimeError("No Mach-O binaries found in the package.")
    if not all(path.resolve() in checked for path in required):
        raise RuntimeError("A required executable or librssguard is not a checked Mach-O binary.")
    run(["codesign", "--verify", "--deep", "--strict", str(app)])
    print(f"Verified {len(checked)} packaged Mach-O binaries for {architecture}, macOS {minimum_macos}.")


def smoke_environment():
    environment = os.environ.copy()
    # Installed SDKs must not hide missing files in the shipped app.
    for name in list(environment):
        if name.startswith("DYLD_") or name in {
            "QT_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH", "QTDIR",
            "QTWEBENGINEPROCESS_PATH", "QTWEBENGINE_RESOURCES_PATH", "QTWEBENGINE_LOCALES_PATH",
        }:
            environment.pop(name)
    return environment


def diagnose_viewer_failure(probe, directory, environment):
    debugger = shutil.which("lldb")
    if not debugger:
        print("LLDB is unavailable; no viewer backtrace can be collected.", flush=True)
        return
    # Reproduce with a fresh profile and preserve the original validation failure.
    profile = directory / "debugger-profile"
    command = [debugger, "--batch", "--no-lldbinit", "--one-line", "settings set target.disable-aslr false",
               "--one-line", "run", "--one-line-on-crash", "thread backtrace all",
               "--one-line-on-crash", "process kill", "--",
               str(probe), "--data", str(profile), "--debug"]
    print("Collecting a viewer shutdown backtrace with LLDB:", flush=True)
    try:
        (profile / "config").mkdir(parents=True)
        report = subprocess.run(command, text=True, capture_output=True, timeout=60, env=environment)
        print(f"LLDB exit code: {report.returncode}\n{report.stdout}\n{report.stderr}", flush=True)
    except (OSError, subprocess.TimeoutExpired) as error:
        print(f"Could not collect the viewer backtrace: {error}", flush=True)


def smoke_test(app, probe, probe_dependencies, variant, directory):
    environment = smoke_environment()
    executable = app / "Contents/MacOS/rssguard"
    version = run([str(executable), "--data", str(directory / "version-profile"), "--version"], env=environment)
    if not re.search(r"\d+\.\d+\.\d+", version):
        raise RuntimeError(f"The packaged application did not report its version: {version}")

    marker = "RSS Guard packaged extractor smoke test"
    paragraph = f"{marker}. This local article exercises extraction without an HTTP request. "
    html = "<html><head><title>Package test</title></head><body><article><h1>Package test</h1>"
    html += "".join(f"<p>{paragraph * 5}</p>" for _ in range(8)) + "</article></body></html>"
    extracted = run([str(app / "Contents/MacOS/rssguard-article-extractor"), "-t",
                     "https://rssguard.invalid/package-test"], input=json.dumps({"html": html}), env=environment)
    if marker not in extracted:
        raise RuntimeError("The packaged article extractor failed its local article test.")

    # Use the deployed probe against a copy of the mounted app, leaving the release DMG intact.
    copy = directory / "smoke/RSS Guard.app"
    copy.parent.mkdir()
    run(["ditto", str(app), str(copy)])
    deployed_probe = copy / "Contents/MacOS/rssguard-package-smoke"
    shutil.copy2(probe, deployed_probe)
    for name in probe_dependencies:
        check_dependency(name, deployed_probe, copy)
    run(["codesign", "--force", "--sign", "-", str(deployed_probe)])
    profile = directory / "viewer-profile"
    # The encryption key is written before settings create their config directory.
    (profile / "config").mkdir(parents=True)
    try:
        output = run([str(deployed_probe), "--data", str(profile), "--debug"], env=environment)
    except (RuntimeError, subprocess.TimeoutExpired):
        diagnose_viewer_failure(deployed_probe, directory, environment)
        raise
    if f"RSSGUARD_PACKAGE_SMOKE_OK:{variant}" not in output:
        raise RuntimeError(f"The packaged {variant} viewer failed its article test: {output}")
    print(f"Passed native startup, {variant} article rendering and article extraction.")


@contextlib.contextmanager
def mounted_package(package, mountpoint):
    mountpoint.mkdir()
    run(["hdiutil", "attach", str(package), "-readonly", "-nobrowse", "-mountpoint", str(mountpoint)])
    try:
        apps = list(mountpoint.glob("*.app"))
        if len(apps) != 1:
            raise RuntimeError("Expected exactly one app bundle in the DMG.")
        yield apps[0]
    finally:
        run(["hdiutil", "detach", str(mountpoint)])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--architecture", choices=SUFFIXES, required=True)
    parser.add_argument("--minimum-macos", required=True)
    parser.add_argument("--check-binary", type=Path)
    parser.add_argument("--build-directory", type=Path)
    parser.add_argument("--variant", choices=("web", "text"))
    parser.add_argument("--smoke-executable", type=Path)
    args = parser.parse_args()
    version_tuple(args.minimum_macos)
    if args.check_binary:
        inspect_binary(args.check_binary, args.architecture, args.minimum_macos)
        return
    if not args.build_directory or not args.variant or not args.smoke_executable:
        parser.error("Package validation requires --build-directory, --variant and --smoke-executable.")
    if platform.system() != "Darwin" or platform.machine() != args.architecture:
        raise RuntimeError("Package smoke tests require a native runner matching the requested architecture.")
    package = select_package(args.build_directory.resolve(), args.architecture, args.variant)
    probe = args.smoke_executable.resolve(strict=True)
    probe_dependencies = inspect_binary(probe, args.architecture, args.minimum_macos)
    with tempfile.TemporaryDirectory(prefix="rssguard-package-") as temporary:
        directory = Path(temporary)
        with mounted_package(package, directory / "mount") as app:
            validate_bundle(app, args.architecture, args.variant, args.minimum_macos)
            smoke_test(app, probe, probe_dependencies, args.variant, directory)


if __name__ == "__main__":
    main()
