#!/usr/bin/env python3

import glob
import os
import shutil
import subprocess
from pathlib import Path

import call_wrapper

def configure():
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    scintilla_dir = root_dir/"submodules"/"scintilla"
    assert(scintilla_dir.exists() and scintilla_dir.is_dir())
    lexers_dir = scintilla_dir/"lexers"
    
    lexers = glob.glob(f"{str(lexers_dir)}/*")
    for lexer in lexers:
        if (os.path.basename(lexer) != "LexCPP.cxx" ):
            os.remove(lexer)
    
    shutil.copy2(cur_dir/"additional_files"/"scintilla.vcxproj", str(scintilla_dir/"win32") + '/')

if __name__ == '__main__':
    call_wrapper.final_call_decorator(
        "Configuring Scintilla", 
        "Configuring Scintilla: success", 
        "Configuring Scintilla: failure!"
    )(configure)()
