#!/usr/bin/env python3

import argparse
import os
import subprocess
from pathlib import Path
from shutil import rmtree

import call_wrapper

def generate(is_debug = False):
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    
    output_dir = root_dir/"_result"/"html"
    if (output_dir.exists()):
        rmtree(output_dir)
        os.makedirs(output_dir)
    
    jsdocs = [str(root_dir/"component"/"docs"/f) for f in ["js/foo_spider_monkey_panel.js","Callbacks.js"]]
    conf = cur_dir/"doc"/"conf.json"
    readme = cur_dir/"doc"/"README.md"
    
    subprocess.check_call(f"jsdoc -t {conf} --readme {readme} {' '.join(jsdocs)} -d {output_dir}", cwd=root_dir, shell=True)
    print(f"Generated docs: {output_dir}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Create HTML from JSDoc (requires `npm`, `jsdoc` and `tui-jsdoc-template`)')
    parser.add_argument('--debug', default=False, action='store_true')

    args = parser.parse_args()

    call_wrapper.final_call_decorator(
        "Generating HTML doc", 
        "Generating HTML doc: success", 
        "Generating HTML doc: failure!"
    )(
    generate
    )(
        args.debug
    )
