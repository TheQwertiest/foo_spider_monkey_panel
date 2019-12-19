#!/usr/bin/env python3

import argparse
import glob
import os
import zipfile
from pathlib import Path
from zipfile import ZipFile

import call_wrapper

def path_basename_tuple(path):
    return (path, os.path.basename(path))

def zipdir(zip, path, arc_path):
    assert(path.exists() and path.is_dir())
    for root, dirs, files in os.walk(path):
        for file in files:
            if (file.startswith(".")):
                # skip `hidden` files
                continue;
            file_path = f"{root}/{file}";
            if (len(arc_path)):
                file_arc_path = f"{arc_path}/{os.path.relpath(file_path, path)}"
            else:
                file_arc_path = os.path.relpath(file_path, path)
            zip.write(file_path, file_arc_path)

def pack(is_debug = False):
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    result_machine_dir = root_dir/"_result"/("Win32_Debug" if is_debug else "Win32_Release")
    assert(result_machine_dir.exists() and result_machine_dir.is_dir())
    
    mozjs_machine_dir = root_dir/"mozjs"/("Debug" if is_debug else "Release")/"bin"
    assert(mozjs_machine_dir.exists() and mozjs_machine_dir.is_dir())
    
    output_dir = result_machine_dir
    if (not output_dir.exists()):
        os.makedirs(output_dir)

    component_zip = output_dir/"foo_spider_monkey_panel.fb2k-component"
    if (component_zip.exists()):
        os.remove(component_zip)
    
    with ZipFile(component_zip, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        zipdir(z, root_dir/"component", "")
        zipdir(z, root_dir/"submodules"/"smp_2003", "samples/complete")
        
        z.write(*path_basename_tuple(root_dir/"LICENSE"))
        z.write(*path_basename_tuple(root_dir/"CHANGELOG.md"))
        
        z.write(*path_basename_tuple(result_machine_dir/"bin"/"foo_spider_monkey_panel.dll"))
        for f in glob.glob(str(mozjs_machine_dir) + "/*.dll"):
            z.write(*path_basename_tuple(f))
        
        if (is_debug):
            # Only debug package should have pdbs inside
            z.write(*path_basename_tuple(result_machine_dir/"dbginfo"/"foo_spider_monkey_panel.pdb"))
            for f in glob.glob(str(mozjs_machine_dir) + "/*.pdb"):
                z.write(*path_basename_tuple(f))
        
        doc_dir = root_dir/"_result"/"html"
        if (doc_dir.exists()):
            zipdir(z, doc_dir, "docs/html")
        else:
            print("No docs found. Skipping...")
    
    print(f"Generated file: {component_zip}")
    
    if (not is_debug):
        # Release pdbs are packed in a separate package
        pdb_zip = output_dir/"foo_spider_monkey_panel_pdb.zip"
        if (pdb_zip.exists()):
            os.remove(pdb_zip)
        
        with ZipFile(pdb_zip, "w", zipfile.ZIP_DEFLATED) as z:
            z.write(*path_basename_tuple(result_machine_dir/"dbginfo"/"foo_spider_monkey_panel.pdb"))
            for f in glob.glob(str(mozjs_machine_dir) + "/*.pdb"):
                z.write(*path_basename_tuple(f))
        
        print(f"Generated file: {pdb_zip}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Pack component to .fb2k-component')
    parser.add_argument('--debug', default=False, action='store_true')

    args = parser.parse_args()

    call_wrapper.final_call_decorator(
        "Packing component", 
        "Packing component: success", 
        "Packing component: failure!"
    )(
    pack
    )(
        args.debug
    )
