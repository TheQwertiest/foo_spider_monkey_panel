#!/usr/bin/env python3

import os
import urllib.request 
from pathlib import Path

print('Downloading new default layout')

cur_dir = Path(__file__).parent.absolute()
root_dir = cur_dir.parent
layouts_dir = root_dir/"_layouts"
if (not layouts_dir.exists()):
    os.makedirs(layouts_dir)

output_file = layouts_dir/"default.html"

urllib.request.urlretrieve("https://raw.githubusercontent.com/pmarsceill/just-the-docs/master/_layouts/default.html", output_file)

print(f"File downloaded: {output_file}")

with open(output_file, "a") as f:
    f.write("<!-- build-commit-id: {{ site.github.build_revision }} -->\n")

print(f"File updated: {output_file}")
