#!/usr/bin/env python3

import subprocess
from pathlib import Path

import call_wrapper

def configure():
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent
    lexilla_dir = root_dir/"submodules"/"lexilla"
    assert(lexilla_dir.exists() and lexilla_dir.is_dir())
    lexers_dir = lexilla_dir/"lexers"

    for lexer in lexers_dir.glob("*"):
        if (lexer.name != "LexCPP.cxx" ):
            lexer.unlink()

    subprocess.check_call("py LexillaGen.py", cwd=lexilla_dir/"scripts", shell=True)

if __name__ == '__main__':
    call_wrapper.final_call_decorator(
        "Configuring Lexilla",
        "Configuring Lexilla: success",
        "Configuring Lexilla: failure!"
    )(configure)()
