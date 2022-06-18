#!/usr/bin/env python3

import shutil
from pathlib import Path

import call_wrapper
import generate_project_filters

def prepare():
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent

    projects = [
        ('mozjs_debug.vcxitems', root_dir/'mozjs'/'Debug'),
        ('mozjs_release.vcxitems', root_dir/'mozjs'/'Release'),
        ('scintilla.vcxproj', root_dir/'submodules'/'scintilla'/'win32'),
        ('lexilla.vcxproj', root_dir/'submodules'/'scintilla'/'lexilla'/'src'),
        ('wtl.vcxitems', root_dir/'submodules'/'WTL')
    ]

    for project_file, project_dir in projects:
        shutil.copy2(cur_dir/'additional_files'/project_file, str(project_dir) + '/')
        generate_project_filters.generate(project_dir/project_file)

if __name__ == '__main__':
    call_wrapper.final_call_decorator(
        'Preparing project files',
        'Preparing: success',
        'Preparing: failure!'
    )(prepare)()
