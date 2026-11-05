"""Validate the staged Windows package and compile its owned paths into NSIS macros."""

import argparse
import os
from pathlib import Path
import re
import stat


PLUGINS = ("feedly", "gmail", "greader", "nextcloud", "standard", "ttrss")


def quoted(value):
    # File source names and !define values are compile-time strings; dollar signs are literal there.
    return '"' + str(value).replace('"', '$\\"') + '"'


def read_definitions(path):
    text = path.read_text(encoding="utf-8-sig")
    values = {}
    for name in ("EXE_NAME", "PLUGIN_XMPP_ENABLED"):
        match = re.search(r'^!define\s+' + name + r'\s+"([^"\r\n]+)"\s*$', text, re.MULTILINE)
        if not match:
            raise ValueError(f"Missing or invalid {name} in {path}")
        values[name] = match[1]
    if values["PLUGIN_XMPP_ENABLED"] not in ("0", "1"):
        raise ValueError("PLUGIN_XMPP_ENABLED must be 0 or 1")
    if Path(values["EXE_NAME"]).name != values["EXE_NAME"] or not values["EXE_NAME"].endswith(".exe"):
        raise ValueError("EXE_NAME must be an executable filename")
    return values["EXE_NAME"], values["PLUGIN_XMPP_ENABLED"] == "1"


def payload_files(stage, executable, xmpp):
    if (not stage.is_dir() or stage.is_symlink() or
            getattr(stage.lstat(), "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT):
        raise ValueError(f"Missing or invalid package directory: {stage}")
    def walk_error(error):
        raise error

    files, seen = [], set()
    for directory, directories, names in os.walk(stage, followlinks=False, onerror=walk_error):
        for name in directories + names:
            entry = Path(directory) / name
            relative = entry.relative_to(stage)
            if relative.parts[0].lower().startswith("data"):
                raise ValueError(f"User data must not be packaged: {relative}")
            if relative.parts[0].lower() in (".rssguard-install.ini", "uninstall.exe"):
                raise ValueError(f"Installer recovery files must not be packaged: {relative}")
            if entry.is_symlink() or getattr(entry.lstat(), "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT:
                raise ValueError(f"Package contains a link or reparse point: {relative}")
            if any(ord(c) < 32 or c in '<>:"|?*\\' for c in name) or name.endswith((" ", ".")):
                raise ValueError(f"Invalid Windows package path: {relative}")
            key = relative.as_posix().casefold()
            if key in seen:
                raise ValueError(f"Case-colliding package path: {relative}")
            seen.add(key)
            # Symbols and MSVC import libraries stay in the portable archive only.
            if entry.is_file() and entry.suffix.lower() not in (".pdb", ".lib"):
                files.append(relative)
    plugins = set(PLUGINS) | ({"xmpp"} if xmpp else set())
    required = {executable, "rssguard.dll", "rssguard-article-extractor.exe", "platforms/qwindows.dll"}
    required.update(f"plugins/rssguard-{name}.dll" for name in plugins)
    actual = {path.as_posix() for path in files}
    if required - actual:
        raise ValueError("Missing package files: " + ", ".join(sorted(required - actual)))
    for path in files:
        if path.parts[0].lower() == "plugins" and path.as_posix() not in required:
            raise ValueError(f"Unexpected service plugin: {path}")
    return sorted(files, key=lambda path: path.as_posix())


def generate(stage, definitions, output):
    executable, xmpp = read_definitions(definitions)
    files = payload_files(stage, executable, xmpp)
    directories = sorted({parent for path in files for parent in path.parents if parent != Path(".")},
                         key=lambda path: (len(path.parts), path.as_posix()))
    lines = ["; Generated from the validated package. Do not edit.", "!macro InstallCoreFiles"]
    current = None
    for path in files:
        if path.parts[0] == "plugins":
            continue
        if current != path.parent:
            current = path.parent
            suffix = "" if current == Path(".") else "\\" + str(current).replace("/", "\\")
            lines += ['  SetOutPath "$INSTDIR' + suffix.replace('$', '$$') + '"',
                      '  !insertmacro CheckErrors "Cannot create the application directory."']
        lines += ["  File " + quoted(stage / path),
                  '  !insertmacro CheckErrors "Cannot install application files. Close RSS Guard and retry."']
    lines += ["!macroend", "", "!macro CheckPayloadDirectories Prefix"]
    for path in directories:
        lines += ['  Push "$INSTDIR\\' + str(path).replace("/", "\\").replace('$', '$$') + '"',
                  "  Call ${Prefix}CheckDirectory"]
    lines += ["!macroend", "", "!macro RemoveInstalledFiles"]
    # The old extensionless helper is an explicitly owned legacy filename.
    for path in [*files, Path("rssguard-article-extractor")]:
        target = '"$INSTDIR\\' + str(path).replace("/", "\\").replace('$', '$$') + '"'
        lines += ["  ClearErrors", "  Delete " + target,
                  '  !insertmacro CheckErrors "Cannot remove application files. Close RSS Guard and retry uninstallation."']
    for path in reversed(directories):
        # Non-empty directories belong to the user too. Never recursively remove them.
        lines += ['  RMDir "$INSTDIR\\' + str(path).replace("/", "\\").replace('$', '$$') + '"']
    lines += ["!macroend", ""]
    output.write_text("\n".join(lines), encoding="utf-8", newline="\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", type=Path, required=True)
    parser.add_argument("--definitions", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        generate(args.stage.absolute(), args.definitions, args.output)
    except (OSError, ValueError) as error:
        parser.exit(1, f"Invalid Windows package: {error}\n")
