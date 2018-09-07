@echo off
setlocal

set ROOT_DIR=%~dp0\..

@echo on
call load_submodules.bat %ROOT_DIR%
call patch_submodules.bat %ROOT_DIR%
