@echo off
setlocal

set ROOT_DIR=%~dp0..\
if not '%1'=='--skip_mozjs' (
    set ROOT_DIR=%1
)

set SKIP_MOZJS=
if '%1'=='--skip_mozjs' (
    set SKIP_MOZJS=1
)
if '%2'=='--skip_mozjs' (
    set SKIP_MOZJS=1
)

echo Preparing project repo

call fetch_submodules.bat %ROOT_DIR%
if errorlevel 1 goto fail
call prepare_scintilla.bat %ROOT_DIR%
if errorlevel 1 goto fail
call patch_submodules.bat %ROOT_DIR%
if errorlevel 1 goto fail
if '%SKIP_MOZJS%'=='' (
    call unpack_mozjs.bat %ROOT_DIR%
    if errorlevel 1 goto fail
)

echo Setup complete!
exit /b 0

:fail
echo Setup failed!
exit /b 1