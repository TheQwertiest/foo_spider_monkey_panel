#!/usr/bin/env python3

import json
import subprocess
from pathlib import Path

import call_wrapper

def generate():
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    output_dir = root_dir/"_result"/"AllPlatforms"/"generated"
    output_dir.mkdir(parents=True, exist_ok=True)

    output_file = output_dir/'doc_package_info.json'
    output_file.unlink(missing_ok=True)

    tag = subprocess.check_output("git describe --tags", shell=True).decode('ascii').strip()
    if (tag.startswith('v')):
        # JsDoc template adds `v` to the version, hence the removal
        tag = tag[1:]

    data = {
        "name": "Spider Monkey Panel",
        "version": tag
    }

    with open(output_file, 'w') as output:
        json.dump(data, output)

    print(f"Generated file: {output_file}")

if __name__ == '__main__':
    call_wrapper.final_call_decorator(
        "Generating package info for docs",
        "Generating package info: success",
        "Generating package info: failure!"
    )(generate)()
