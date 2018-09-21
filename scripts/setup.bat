@echo off
setlocal

set ROOT_DIR=%~dp0..\

call fetch_submodules.bat %ROOT_DIR%
call prepare_scintilla.bat %ROOT_DIR%
call patch_submodules.bat %ROOT_DIR%
if not '%1'=='--skip_mozjs' (
    call prepare_mozjs.bat %ROOT_DIR%
)
