#!/usr/bin/env python3

import argparse
import os
import subprocess
from pathlib import Path
from shutil import rmtree
from urllib.parse import quote

def get_cwd_repo_root():
    repo_dir = Path(os.getcwd()).absolute()
    cmd_get_root = [
        'git',
        '-C',
        str(repo_dir),
        'rev-parse',
        '--show-toplevel'
    ]

    root = subprocess.check_output(cmd_get_root, text=True, env=os.environ, cwd=repo_dir).strip()
    return Path(root)

def generate(root_dir, component_name):
    if not root_dir:
        root_dir = get_cwd_repo_root()

    license_dir = root_dir/'assets'/'generated_files'/'licenses'

    output_file = root_dir/'docs'/'third_party_notices.md'
    output_file.unlink(missing_ok=True)

    with open(license_dir/'.license_index.txt') as f:
        index = [l.strip().split(': ') for l in f.readlines() if l.strip()]

    licenses = [f for f in license_dir.glob('*.txt') if f.stem != '.license_index']

    if set([l[0] for l in index]) != set([l.stem for l in licenses]):
        raise RuntimeError('License file mismatch:\n'
                           f'Index: {set([l for l in index[0]]) - set([l.stem for l in licenses])}\n'
                           f'Files: {set([l.stem for l in licenses]) - set([l for l in index[0]])}')

    with open(output_file, 'w') as output:
        output.write('---\n')
        output.write('layout: default\n')
        output.write('title: Third party notices\n')
        output.write('nav_exclude: true\n')
        output.write('---\n')
        output.write('\n')
        output.write('# Third party notices\n')
        output.write('{: .no_toc }\n')
        output.write('\n')
        output.write('---\n')
        output.write('\n')
        output.write(f'{component_name} uses third-party libraries or other resources that may\n')
        output.write(f'be distributed under licenses different than the {component_name} software.\n')
        output.write('\n')
        output.write('The linked notices are provided for information only.\n')
        output.write('\n')
        for (dep_name, license) in index:
            output.write(f'- [{dep_name} - {license}]({{{{site.baseurl}}}}assets/generated_files/licenses/{dep_name}.txt)\n')

    print(f'Generated file: {output_file}')

generate(None, 'Spider Monkey Panel')
