#!/usr/bin/env python3

import argparse
import json
import os
import subprocess
from pathlib import Path
from pathlib import PureWindowsPath
from typing import Union

PathLike = Union[str, Path]

def generate_config( base_dir: PathLike,
                     output_dir: PathLike,
                     repo: str,
                     commit_hash: str ):
    base_dir = Path(base_dir).resolve()
    assert(base_dir.exists() and base_dir.is_dir())
    
    output_dir = Path(output_dir).resolve()
    if (not output_dir.exists()):
        os.makedirs(output_dir)
    
    output_file = output_dir/'source_link.json'
    if (output_file.exists()):
        os.remove(output_file)
    
    data = {}
    data['documents'] = { f'{PureWindowsPath(base_dir)}*': f'https://raw.githubusercontent.com/{repo}/{commit_hash}/*' }    
    
    with open(output_file, 'w') as output:
        json.dump(data, output)

if __name__ == '__main__':
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    output_dir = cur_dir/"_result"/"AllPlatforms"/"generated"
    commit_hash = subprocess.check_output("git rev-parse --short HEAD", shell=True).decode('ascii')

    parser = argparse.ArgumentParser(description='Generate source link configuration file')
    parser.add_argument('--base_dir', default=root_dir)
    parser.add_argument('--output_dir', default=output_dir)
    parser.add_argument('--commit_hash', default=commit_hash)
    parser.add_argument('--repo', default="theqwertiest/foo_spider_monkey_panel")

    args = parser.parse_args()

    generate_config(args.base_dir, args.output_dir, args.repo, args.commit_hash)
