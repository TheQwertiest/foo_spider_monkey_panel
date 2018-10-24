@echo off
setlocal

set CUR_DIR=%~dp0
set ROOT_DIR=%1
if '%ROOT_DIR%'=='' set ROOT_DIR=%CUR_DIR%..\

echo Patching submodules

cd %ROOT_DIR%

git apply %CUR_DIR%patches\columns_ui-sdk.patch %CUR_DIR%patches\scintilla.patch
if errorlevel 1 goto fail
exit /b 0

:fail
echo Failed!
exit /b 1