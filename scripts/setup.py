#!/usr/bin/env python3

import argparse
import json
import subprocess
import sys
import traceback
from pathlib import Path
from pathlib import PureWindowsPath
from typing import Union

import call_wrapper
import download_submodules
import configure_scintilla
import patch_submodules
import unpack_mozjs
import patch_mozjs
import generate_commit_hash_header
import generate_source_link_config

PathLike = Union[str, Path]

def call_decorator(command_name: str):
    def f_decorator(f):
        def wrapper(*args, **kwds):
            print(f">> {command_name}: starting")
            try:
                f(*args, **kwds)
                print(f"<< {command_name}: success")
            except Exception:
                traceback.print_exc(file=sys.stderr)
                print(f"<< {command_name}: failure", file=sys.stderr)
                raise call_wrapper.SkippedError()
        
        return wrapper

    return f_decorator

def setup( skip_mozjs,
           skip_submodules_download,
           skip_submodules_patches ):
    cur_dir = Path(__file__).parent.absolute()
    
    if (not skip_submodules_download):
        call_decorator("Downloading submodules")(download_submodules.download)();
        call_decorator("Configuring Scintilla")(configure_scintilla.configure)();
        if (not skip_submodules_patches):
            call_decorator("Patching submodules")(patch_submodules.patch)();
    if (not skip_mozjs):
        call_decorator("MozJs unpacking")(unpack_mozjs.unpack)();
        # call_decorator("MozJs patching")(patch_mozjs.patch)();
    call_decorator("Commit hash header generation")(generate_commit_hash_header.generate_header)();
    call_decorator("SourceLink configuration file generation")(generate_source_link_config.generate_config)();

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Setup project')
    parser.add_argument('--skip_mozjs', default=False, action='store_true')
    parser.add_argument('--skip_submodules_download', default=False, action='store_true')
    parser.add_argument('--skip_submodules_patches', default=False, action='store_true')

    args = parser.parse_args()

    call_wrapper.final_call_decorator(
        "Preparing project repo", 
        "Setup complete!", 
        "Setup failed!"
    )(
    setup
    )(
        args.skip_mozjs, 
        args.skip_submodules_download, 
        args.skip_submodules_patches
    )

