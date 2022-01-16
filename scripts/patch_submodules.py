#!/usr/bin/env python3

import subprocess
from pathlib import Path

import call_wrapper

def patch():
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    patches = [cur_dir/"patches"/p for p in ["scintilla.patch", "smp_2003.patch", "mozjs.patch", "cui.patch", "fb2k_utils.patch"]]
    for p in patches:
        assert(p.exists() and p.is_file())

    subprocess.check_call(f"git apply --ignore-whitespace {' '.join(str(p) for p in patches)}", cwd=root_dir, shell=True)

if __name__ == '__main__':
    call_wrapper.final_call_decorator(
        "Patching submodules",
        "Patching submodules: success",
        "Patching submodules: failure!"
    )(patch)()
