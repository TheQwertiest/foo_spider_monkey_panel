@echo off
setlocal

set PATH=%PATH%;C:\Program Files\7-Zip
set ROOT_DIR=%~dp0..\
set CONFIGURATION=Release
if '%1'=='--debug' (
    set CONFIGURATION=Debug
)

set COMPONENT_DIR_NO_SLASH=%ROOT_DIR%component
set COMPONENT_DLL=%ROOT_DIR%_result\Win32_%CONFIGURATION%\bin\foo_spider_monkey_panel.dll
set MOZ_JS_BIN_DIR_NO_SLASH=%ROOT_DIR%mozjs\%CONFIGURATION%\bin
set COMPONENT_OUT_DIR_NO_SLASH=%ROOT_DIR%_result\Win32_%CONFIGURATION%\component
set COMPONENT_OUT_DIR=%COMPONENT_OUT_DIR_NO_SLASH%\
set FB2K_ARCHIVE=%ROOT_DIR%foo_spider_monkey_panel.fb2k-component

@echo on

if not exist "%COMPONENT_OUT_DIR_NO_SLASH%" mkdir "%COMPONENT_OUT_DIR_NO_SLASH%"
xcopy /r/y/s "%COMPONENT_DIR_NO_SLASH%" "%COMPONENT_OUT_DIR_NO_SLASH%"
xcopy /r/y/s "%MOZ_JS_BIN_DIR_NO_SLASH%" "%COMPONENT_OUT_DIR_NO_SLASH%"
xcopy /r/y "%COMPONENT_DLL%" "%COMPONENT_OUT_DIR%"

if exist "%FB2K_ARCHIVE%" del /f/q "%FB2K_ARCHIVE%"
7z a -tzip "%FB2K_ARCHIVE%" "%COMPONENT_OUT_DIR%*"
