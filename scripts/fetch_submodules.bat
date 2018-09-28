@echo off
setlocal

set CUR_DIR=%~dp0
set ROOT_DIR=%1

if '%ROOT_DIR%'=='' set ROOT_DIR=%CUR_DIR%..\

echo Downloading submodules

cd %ROOT_DIR%

git submodule foreach git reset --hard
if errorlevel 1 goto fail
git submodule update --init --recursive
if errorlevel 1 goto fail
exit /b 0

:fail
echo Failed!
exit /b 1
