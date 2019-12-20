#!/usr/bin/env python3

import json
import os
import subprocess
from pathlib import Path

import call_wrapper

def generate():
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    output_dir = root_dir/"_result"/"AllPlatforms"/"generated"
    if (not output_dir.exists()):
        os.makedirs(output_dir)
    
    output_file = output_dir/'doc_package_info.json'
    if (output_file.exists()):
        os.remove(output_file)
    
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
