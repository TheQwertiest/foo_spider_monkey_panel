@echo off
setlocal

set CUR_DIR=%~dp0
set ROOT_DIR=%1
if '%ROOT_DIR%'=='' set ROOT_DIR=%CUR_DIR%..\

echo Patching mozjs

cd %ROOT_DIR%

git apply --ignore-whitespace %CUR_DIR%patches\mozjs.patch
if errorlevel 1 goto fail
exit /b 0

:fail
echo Failed!
exit /b 1