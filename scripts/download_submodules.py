#!/usr/bin/env python3

import argparse
import os
from pathlib import Path
import subprocess
import urllib.request as ureq

import call_wrapper

def get_submodules_urls(root_dir):
    config_pairs = (
        subprocess.check_output('git config --file .gitmodules --get-regexp "submodule\..*\.url"', cwd=root_dir, text=True, shell=True)
        .splitlines()
    )
    name_to_value = {i.split(' ')[0][len('submodule.submodules/'):-len('.url')]: i.split(' ')[1] for i in config_pairs}

    return name_to_value

def download_submodule(root_dir, submodule_name, submodule_url):
    print(f"Downloading {submodule_name}...") 
       
    with ureq.urlopen(f'{submodule_url}/info/refs?service=git-upload-pack') as response:
        _ = response.readline()
        features = response.readline().decode('utf-8').split(' ')
        supports_filters = 'filter' in features
    
    if supports_filters:
        subprocess.check_call(f"git submodule update --init --filter=blob:none -- submodules/{submodule_name}", cwd=root_dir, shell=True)
    else:
        try:
            subprocess.check_call(f"git submodule update --init --depth=10 -- submodules/{submodule_name}", cwd=root_dir, shell=True)
        except subprocess.CalledProcessError:
            try:
                subprocess.check_call(f"git submodule update --init --depth=50 -- submodules/{submodule_name}", cwd=root_dir, shell=True)
            except subprocess.CalledProcessError:
                # Shallow copy does not honour default branch config
                subprocess.check_call("git config --add remote.origin.fetch +refs/heads/*:refs/remotes/origin/*",
                                      cwd=root_dir/"submodules"/submodule_name,
                                      shell=True)
                subprocess.check_call(f"git submodule deinit --force -- submodules/{submodule_name}", cwd=root_dir, shell=True)
                subprocess.check_call(f"git submodule update --init --force -- submodules/{submodule_name}", cwd=root_dir, shell=True)
    

def download(root_dir):
    if root_dir:
        root_dir = Path(root_dir)
    else:
        cur_dir = Path(__file__).parent.absolute()
        root_dir = cur_dir.parent

    submodule_urls = get_submodules_urls(root_dir)
    subprocess.check_call("git submodule sync", cwd=root_dir, shell=True)
    subprocess.check_call("git submodule foreach git reset --hard", cwd=root_dir, shell=True)
    
    for subdir in [f for f in (root_dir/"submodules").iterdir() if f.is_dir()]:
        download_submodule(root_dir, subdir.name, submodule_urls[subdir.name])

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Download submodules')
    parser.add_argument('--root_dir', default=Path(os.getcwd()).absolute())

    args = parser.parse_args()

    call_wrapper.final_call_decorator(
        "Downloading submodules",
        "Downloading submodules: success",
        "Downloading submodules: failure!"
    )(download)(args.root_dir)
