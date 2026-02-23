#!/usr/bin/env python3

import re
from pathlib import Path


def extract_icon_names(src_dir: Path):
    pattern = re.compile(
        r'fromTheme\(QSL\("([^"]+)"\)(?:,\s*QSL\("([^"]+)"\))?'
    )

    icons = set()

    # Limit to likely source files for speed
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

        for icon in icon_names:
            # Equivalent to:
            # "$THEME_DIR"/*/*/"$ICON".{svg,png}

            for category_dir in theme_dir.iterdir():
                if not category_dir.is_dir():
                    continue

                for size_dir in category_dir.iterdir():
                    if not size_dir.is_dir():
                        continue

                    for ext in ("svg", "png"):
                        candidate = size_dir / f"{icon}.{ext}"
                        if candidate.exists():
                            rel = candidate.relative_to(root / "resources")
                            print(f"    <file>./{rel.as_posix()}</file>")
                            break
                    else:
                        continue
                    break
                else:
                    continue
                break

        # Append index.theme
        index_path = (theme_dir / "index.theme").relative_to(root / "resources")
        print(f"    <file>./{index_path.as_posix()}</file>")

    print("  </qresource>")
    print("</RCC>")


if __name__ == "__main__":
    main()