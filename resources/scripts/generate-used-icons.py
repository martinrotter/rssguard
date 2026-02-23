#!/usr/bin/env python3

import re
from pathlib import Path


def extract_icon_names(src_dir: Path):
    """
    Extract icon names from:
      fromTheme(QSL("icon"))
      fromTheme(QSL("icon1"), QSL("icon2"))
    """

    pattern = re.compile(
        r'fromTheme\(QSL\("([^"]+)"\)(?:,\s*QSL\("([^"]+)"\))?'
    )

    icons = set()

    for ext in ("*.cpp", "*.cc", "*.cxx", "*.h", "*.hpp"):
        for path in src_dir.rglob(ext):
            try:
                text = path.read_text(encoding="utf-8", errors="ignore")
            except Exception:
                continue

            for match in pattern.finditer(text):
                icons.add(match.group(1))
                if match.group(2):
                    icons.add(match.group(2))

    return sorted(icons)


def index_theme(theme_dir: Path, root: Path):
    """
    Build dictionary:
        icon_name -> relative resource path
    First match wins (same as your bash glob logic).
    """

    index = {}

    for file in theme_dir.rglob("*"):
        if not file.is_file():
            continue

        if file.name == "index.theme":
            continue

        if file.suffix.lower() not in (".svg", ".png"):
            continue

        icon_name = file.stem

        # Only keep first occurrence (mimics bash "break").
        if icon_name not in index:
            rel = file.relative_to(root / "resources")
            index[icon_name] = rel.as_posix()

    return index


def main():
    root = Path.cwd()
    src = root / "src"
    themes_root = root / "resources" / "graphics"
    icon_themes = ["Breeze", "Breeze Dark"]

    icon_names = extract_icon_names(src)

    print("<RCC>")
    print('  <qresource prefix="/">')

    for theme in icon_themes:
        theme_dir = themes_root / theme

        # Index theme ONCE.
        theme_index = index_theme(theme_dir, root)

        for icon in icon_names:
            if icon in theme_index:
                print(f"    <file>./{theme_index[icon]}</file>")

        # Always include index.theme.
        index_path = (theme_dir / "index.theme").relative_to(root / "resources")
        print(f"    <file>./{index_path.as_posix()}</file>")

    print("  </qresource>")
    print("</RCC>")


if __name__ == "__main__":
    main()