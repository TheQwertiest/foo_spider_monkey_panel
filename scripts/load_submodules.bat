@echo off
setlocal

set CUR_DIR=%~dp0
set ROOT_DIR=%1

if '%ROOT_DIR%'=='' set ROOT_DIR=%CUR_DIR%..\

@echo on
cd %ROOT_DIR%
git submodule foreach git reset --hard
git submodule update --init
git submodule update --recursive --remote
