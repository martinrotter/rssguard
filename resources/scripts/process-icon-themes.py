#!/usr/bin/env python3

import os
import re
import shutil
from collections import defaultdict

ROOTS = [
    r"./resources/graphics/Breeze",
    r"./resources/graphics/Breeze Dark"
]

NEW_ICONS = [
    ("go-down", "download"),
    ("go-bottom", "download-all")
]

size_pattern = re.compile(r"^(\d+)(x\d+)?$")

for ROOT in ROOTS:

    print("\n====================================")
    print("Processing theme:", ROOT)
    print("====================================")

    # ------------------------------------------------
    # 1. Remove retina folders
    # ------------------------------------------------

    print("\nRemoving retina folders...")

    for dirpath, dirnames, filenames in os.walk(ROOT, topdown=True):
        for d in list(dirnames):
            if d.endswith("2x") or d.endswith("3x") or "@2x" in d or "@3x" in d:
                full = os.path.join(dirpath, d)
                print("Removing directory:", full)
                shutil.rmtree(full)
                dirnames.remove(d)

    # ------------------------------------------------
    # 2. Resolve symlinks
    # ------------------------------------------------

    print("\nResolving symlinks...")

    for dirpath, dirnames, filenames in os.walk(ROOT):
        for name in filenames + dirnames:
            path = os.path.join(dirpath, name)

            if not os.path.islink(path):
                continue

            target = os.path.realpath(path)

            if os.path.isdir(target):
                print(f"Removing directory symlink {path}")
                os.unlink(path)
                continue

            if os.path.isfile(target):
                print(f"Replacing file symlink {path}")
                os.unlink(path)
                shutil.copy2(target, path)
                continue

            print("Broken symlink:", path)

    # ------------------------------------------------
    # 3. Collect icons
    # ------------------------------------------------

    print("\nCollecting icons...")

    icons = defaultdict(list)

    for dirpath, dirnames, filenames in os.walk(ROOT):
        parts = dirpath.split(os.sep)

        size = None
        category = None

        for p in parts:
            m = size_pattern.match(p)
            if m:
                size = int(m.group(1))
                break

        if size is None:
            continue

        try:
            idx = parts.index(str(size))
            category = parts[idx - 1]
        except:
            category = "unknown"

        for f in filenames:
            if not f.endswith((".png", ".svg")):
                continue

            full = os.path.join(dirpath, f)
            key = (category, f)

            icons[key].append((size, full))

    # ------------------------------------------------
    # 4. Deduplicate
    # ------------------------------------------------

    print("\nRemoving smaller duplicates...")

    deleted = 0

    for key, versions in icons.items():

        svg_versions = [v for v in versions if v[1].endswith(".svg")]
        if svg_versions:
            versions = svg_versions

        versions.sort(reverse=True)

        keep_size, keep_path = versions[0]

        for size, path in versions[1:]:
            print(f"Removing {path}")
            os.remove(path)
            deleted += 1

    print("Deleted files:", deleted)

    # ------------------------------------------------
    # 5. Move icons into single folder
    # ------------------------------------------------

    icons_dir = os.path.join(ROOT, "icons")
    os.makedirs(icons_dir, exist_ok=True)

    print("\nMoving icons to /icons ...")

    for dirpath, dirnames, filenames in os.walk(ROOT):
        if dirpath.startswith(icons_dir):
            continue

        for f in filenames:
            if not f.endswith((".png", ".svg")):
                continue

            src = os.path.join(dirpath, f)
            dst = os.path.join(icons_dir, f)

            base, ext = os.path.splitext(f)
            i = 1

            while os.path.exists(dst):
                dst = os.path.join(icons_dir, f"{base}_{i}{ext}")
                i += 1

            print("Move:", src, "->", dst)
            shutil.move(src, dst)

    # ------------------------------------------------
    # 6. Remove leftover folders
    # ------------------------------------------------

    print("\nCleaning leftover folders...")

    for entry in os.listdir(ROOT):
        full = os.path.join(ROOT, entry)

        if entry == "icons" or entry == "index.theme":
            continue

        if os.path.isdir(full):
            print("Removing:", full)
            shutil.rmtree(full)

        elif os.path.isfile(full) and entry != "index.theme":
            os.remove(full)

    # ------------------------------------------------
    # 7. Generate new index.theme
    # ------------------------------------------------

    theme_name = os.path.basename(ROOT)

    index_path = os.path.join(ROOT, "index.theme")

    print("\nGenerating new index.theme")

    content = f"""[Icon Theme]
Name={theme_name}
Comment=Custom icon theme
Directories=icons
Inherits=hicolor

[icons]
Size=48
Type=Scalable
Context=Applications
MinSize=16
MaxSize=512
"""

    with open(index_path, "w", encoding="utf-8") as f:
        f.write(content)

    print("Generated:", index_path)

    # ------------------------------------------------
    # 8. Generate additional icons
    # ------------------------------------------------

    print("\nGenerating additional icons...")

    for src_name, dst_name in NEW_ICONS:

        for ext in (".svg", ".png"):

            src = os.path.join(icons_dir, src_name + ext)
            dst = os.path.join(icons_dir, dst_name + ext)

            if not os.path.exists(src):
                continue

            print("Create:", dst, "from", src)
            shutil.copy2(src, dst)

print("\nAll themes processed.")