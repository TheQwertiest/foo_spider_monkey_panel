@echo off
setlocal

set ROOT_DIR=%~dp0..\

if not '%1'=='--patch_only' (
    @echo on
    call load_submodules.bat %ROOT_DIR%
    @echo off
)

if not '%1'=='--init_only' (
    @echo on
    if not '%1'=='--skip_mozjs' (
        call prepare_mozjs.bat %ROOT_DIR%
    )
    call prepare_scintilla.bat %ROOT_DIR%
    call patch_submodules.bat %ROOT_DIR%
)
