@echo off
setlocal

set CUR_DIR=%~dp0
set ROOT_DIR=%CUR_DIR%..\
if not '%1'=='' if not '%1'=='--skip_mozjs' (
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

call %CUR_DIR%fetch_submodules.bat %ROOT_DIR%
if errorlevel 1 goto fail
call %CUR_DIR%prepare_scintilla.bat %ROOT_DIR%
if errorlevel 1 goto fail
call %CUR_DIR%patch_submodules.bat %ROOT_DIR%
if errorlevel 1 goto fail
if '%SKIP_MOZJS%'=='' (
    call %CUR_DIR%unpack_mozjs.bat %ROOT_DIR%
    if errorlevel 1 goto fail
	call %CUR_DIR%patch_mozjs.bat %ROOT_DIR%
    if errorlevel 1 goto fail
)
call %CUR_DIR%generate_commit_hash_header.bat %ROOT_DIR%
if errorlevel 1 goto fail

echo Setup complete!
exit /b 0

:fail
echo Setup failed!
exit /b 1