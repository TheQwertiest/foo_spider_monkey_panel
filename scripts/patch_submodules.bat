@echo off
setlocal

set CUR_DIR=%~dp0
set ROOT_DIR=%1

if '%ROOT_DIR%'=='' set ROOT_DIR=%CUR_DIR%..\

@echo on
cd %ROOT_DIR%
git apply %CUR_DIR%patches\columns_ui-sdk.patch %CUR_DIR%patches\pfc.patch %CUR_DIR%patches\foobar2000.patch
