#!/usr/bin/env python3

import subprocess
from pathlib import Path

import call_wrapper

def configure():
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    scintilla_dir = root_dir/"submodules"/"scintilla"
    assert(scintilla_dir.exists() and scintilla_dir.is_dir())
    lexers_dir = scintilla_dir/"lexers"

    for lexer in lexers_dir.glob("*"):
        if (lexer.name != "LexCPP.cxx" ):
            lexer.unlink()

    subprocess.check_call("py LexGen.py", cwd=scintilla_dir/"scripts", shell=True)
    subprocess.check_call("py LexillaGen.py", cwd=scintilla_dir/"lexilla"/"scripts", shell=True)

if __name__ == '__main__':
    call_wrapper.final_call_decorator(
        "Configuring Scintilla",
        "Configuring Scintilla: success",
        "Configuring Scintilla: failure!"
    )(configure)()
