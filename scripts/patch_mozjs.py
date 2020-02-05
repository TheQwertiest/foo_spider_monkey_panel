#!/usr/bin/env python3

import subprocess
from pathlib import Path

import call_wrapper

def patch():
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    mozjs_patch = cur_dir/"patches"/"mozjs.patch"
    assert(mozjs_patch.exists() and mozjs_patch.is_file())
    
    subprocess.check_call(f"git apply --ignore-whitespace {mozjs_patch}", cwd=root_dir, shell=True)

if __name__ == '__main__':
    call_wrapper.final_call_decorator(
        "Patching mozjs",
        "Patching mozjs: success",
        "Patching mozjs: failure!"
    )(patch)()
