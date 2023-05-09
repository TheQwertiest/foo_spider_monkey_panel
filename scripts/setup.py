#!/usr/bin/env python3

import argparse
import importlib
import importlib.util
import sys
import traceback
from pathlib import Path
from typing import Union

import call_wrapper
import configure_lexilla
import configure_scintilla
import download_submodules
import patch_submodules
import prepare_submodule_projects
import unpack_mozjs

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

def load_module(script_path):
    spec = importlib.util.spec_from_file_location("module.name", script_path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod

def setup( skip_mozjs,
           skip_submodules_download,
           skip_submodules_patches ):
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    scripts_path = root_dir/'submodules'/'fb2k_utils'/'scripts'

    if (not skip_submodules_download):
        call_decorator('Downloading submodules')(download_submodules.download)(root_dir)
        call_decorator("Configuring Scintilla")(configure_scintilla.configure)()
        call_decorator("Configuring Lexilla")(configure_lexilla.configure)()
        if (not skip_mozjs):
            call_decorator("MozJs unpacking")(unpack_mozjs.unpack)()
        if (not skip_submodules_patches):
            call_decorator('Patching fb2k submodules')(
                load_module(scripts_path/'patch_fb2k_submodules.py').patch
            )(
                root_dir=root_dir
            )
            call_decorator("Patching submodules")(patch_submodules.patch)()
        call_decorator("Preparing submodule projects")(prepare_submodule_projects.prepare)()

    call_decorator('Version header generation')(
        load_module(scripts_path/'generate_version_header.py').generate_header_custom
    )(
        repo_dir=root_dir,
        output_dir=root_dir/'_result'/'AllPlatforms'/'generated',
        component_prefix='SMP'
    )
    call_decorator('Commit hash header generation')(
        load_module(scripts_path/'generate_commit_hash_header.py').generate_header_custom
    )(
        repo_dir=root_dir,
        output_dir=root_dir/'_result'/'AllPlatforms'/'generated',
        component_prefix='SMP'
    )
    call_decorator('SourceLink configuration file generation'
    )(
        load_module(scripts_path/'generate_source_link_config.py').generate_config_custom
    )(
        repo_dir=root_dir,
        output_dir=root_dir/'_result'/'AllPlatforms'/'generated',
        repo='theqwertiest/foo_spider_monkey_panel'
    )
    call_decorator('3rd-party notices generation'
    )(
        load_module(scripts_path/'generate_third_party.py').generate
    )(
        root_dir=root_dir,
        component_name='Spider Monkey Panel'
    )

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
