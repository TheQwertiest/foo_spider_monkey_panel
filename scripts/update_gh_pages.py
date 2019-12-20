#!/usr/bin/env python3

import argparse
import os
import shutil
from pathlib import Path
from typing import Union

import call_wrapper

PathLike = Union[str, Path]

def update(gh_pages_dir: PathLike):
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    
    gh_pages_dir = Path(gh_pages_dir).resolve()
    assert(gh_pages_dir.exists() and gh_pages_dir.is_dir())
    
    ghp_gen_dir = gh_pages_dir/"assets"/"generated_files"
    if (ghp_gen_dir.exists()):
        shutil.rmtree(ghp_gen_dir)
        os.makedirs(ghp_gen_dir)

    shutil.copytree(root_dir/"component"/"licenses", ghp_gen_dir/"licenses", dirs_exist_ok=True)
    shutil.copytree(root_dir/"component"/"samples", ghp_gen_dir/"samples", dirs_exist_ok=True)
    shutil.copytree(root_dir/"submodules"/"smp_2003", ghp_gen_dir/"samples"/"complete", dirs_exist_ok=True)
    shutil.copytree(root_dir/"component"/"docs", ghp_gen_dir/"docs", dirs_exist_ok=True)
    shutil.copytree(root_dir/"_result"/"html", ghp_gen_dir/"docs"/"html", dirs_exist_ok=True)
    
    ghp_inc_dir = gh_pages_dir/"_includes"
    if (not ghp_inc_dir.exists()):
        os.makedirs(ghp_inc_dir)
    
    shutil.copy2(root_dir/"CHANGELOG.md", gh_pages_dir/"_includes"/"CHANGELOG.md")
    shutil.copy2(root_dir/"component"/"samples"/"readme.md", gh_pages_dir/"_includes"/"samples_readme.md")

if __name__ == '__main__':
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    gh_pages_dir = root_dir/"gh-pages"

    parser = argparse.ArgumentParser(description='Update GitHub Pages content')
    parser.add_argument('--gh_pages_dir', default=gh_pages_dir)

    args = parser.parse_args()

    call_wrapper.final_call_decorator(
        "Updating GitHub Pages content", 
        "Updating GitHub Pages content: success", 
        "Updating GitHub Pages content: failure!"
    )(
        update
    )(
        gh_pages_dir
    )
