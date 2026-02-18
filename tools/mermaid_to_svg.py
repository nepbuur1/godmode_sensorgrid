#!/usr/bin/env python3
"""Convert mermaid (.mmd) files to SVG using the Kroki.io online service.

Usage:
    python3 mermaid_to_svg.py <input.mmd> <output.svg>
    python3 mermaid_to_svg.py --all   # Convert all known mermaid files in the project
"""

import sys
import os
import base64
import zlib
import urllib.request
import urllib.error


def mermaid_to_svg(mmd_content: str) -> str:
    """Convert mermaid source to SVG via Kroki.io API."""
    url = "https://kroki.io/mermaid/svg"
    data = mmd_content.encode("utf-8")
    req = urllib.request.Request(
        url,
        data=data,
        headers={
            "Content-Type": "text/plain",
            "User-Agent": "mermaid_to_svg/1.0",
            "Accept": "image/svg+xml",
        },
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=30) as resp:
        svg = resp.read().decode("utf-8")

    # Add white background: insert a white rect right after the opening <svg ...> tag
    insert_pos = svg.find(">", svg.find("<svg")) + 1
    if insert_pos > 0:
        svg = svg[:insert_pos] + '<rect width="100%" height="100%" fill="white"/>' + svg[insert_pos:]

    # Make edge label backgrounds white (semi-transparent)
    svg = svg.replace("rgba(232,232,232, 0.8)", "rgba(255,255,255, 0.8)")
    svg = svg.replace("rgba(232, 232, 232, 0.8)", "rgba(255, 255, 255, 0.8)")
    svg = svg.replace("rgba(232, 232, 232, 0.5)", "rgba(255, 255, 255, 0.5)")

    return svg


def convert_file(input_path: str, output_path: str) -> bool:
    """Convert a single .mmd file to .svg. Returns True on success."""
    try:
        with open(input_path, "r") as f:
            mmd_content = f.read()

        svg_content = mermaid_to_svg(mmd_content)

        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        with open(output_path, "w") as f:
            f.write(svg_content)

        print(f"OK: {input_path} -> {output_path}")
        return True
    except Exception as e:
        print(f"FAIL: {input_path} -> {e}", file=sys.stderr)
        return False


def convert_all():
    """Convert all known mermaid files in the project."""
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    sensorgrid_dirs = []
    apps_root = os.path.join(project_root, "apps")
    for entry in os.listdir(apps_root):
        if entry.startswith("sensorgrid_"):
            sensorgrid_dirs.append(os.path.join(apps_root, entry))

    conversions = []
    for sg_dir in sensorgrid_dirs:
        # Check system-level doc/mermaid folder
        mermaid_dir = os.path.join(sg_dir, "doc", "mermaid")
        img_dir = os.path.join(sg_dir, "doc", "img")
        if os.path.isdir(mermaid_dir):
            for fname in os.listdir(mermaid_dir):
                if fname.endswith(".mmd"):
                    svg_name = fname.replace(".mmd", ".svg")
                    conversions.append((
                        os.path.join(mermaid_dir, fname),
                        os.path.join(img_dir, svg_name),
                    ))

        # Check each app's doc/mermaid folder
        for app_name in os.listdir(sg_dir):
            mermaid_dir = os.path.join(sg_dir, app_name, "doc", "mermaid")
            img_dir = os.path.join(sg_dir, app_name, "doc", "img")
            if os.path.isdir(mermaid_dir):
                for fname in os.listdir(mermaid_dir):
                    if fname.endswith(".mmd"):
                        svg_name = fname.replace(".mmd", ".svg")
                        conversions.append((
                            os.path.join(mermaid_dir, fname),
                            os.path.join(img_dir, svg_name),
                        ))

    if not conversions:
        print("No .mmd files found.")
        return

    success = 0
    for mmd_path, svg_path in sorted(conversions):
        if convert_file(mmd_path, svg_path):
            success += 1

    print(f"\nConverted {success}/{len(conversions)} files.")


if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == "--all":
        convert_all()
    elif len(sys.argv) == 3:
        if not convert_file(sys.argv[1], sys.argv[2]):
            sys.exit(1)
    else:
        print(__doc__)
        sys.exit(1)
