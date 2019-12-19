#!/usr/bin/env python3

import os
import subprocess
from pathlib import Path

import call_wrapper

def download_submodule(root_dir, submodule_name):
    try:
        subprocess.check_call(f"git submodule update --init --depth=10 -- submodules/{submodule_name}", cwd=root_dir, shell=True)
    except subprocess.CalledProcessError as e: 
        try:
            subprocess.check_call(f"git submodule update --init --depth=50 -- submodules/{submodule_name}", cwd=root_dir, shell=True)
        except subprocess.CalledProcessError as e: 
            # Shallow copy does not honour default branch config
            subprocess.check_call(f"git config --add remote.origin.fetch +refs/heads/*:refs/remotes/origin/*", cwd=root_dir/"submodules"/submodule_name, shell=True)
            subprocess.check_call(f"git submodule deinit --force -- submodules/{submodule_name}", cwd=root_dir, shell=True)
            subprocess.check_call(f"git submodule update --init --force -- submodules/{submodule_name}", cwd=root_dir, shell=True)
            # We tried T_T

def download():
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    submodules_dir = root_dir/"submodules"
    
    subprocess.check_call("git submodule sync", cwd=root_dir, shell=True)
    subprocess.check_call("git submodule foreach git reset --hard", cwd=root_dir, shell=True)
    for subdir in os.listdir(submodules_dir):
        download_submodule(root_dir, subdir)

if __name__ == '__main__':
    call_wrapper.final_call_decorator(
        "Downloading submodules", 
        "Downloading submodules: success", 
        "Downloading submodules: failure!"
    )(download)()

