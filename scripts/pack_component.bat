@echo off
setlocal

set PATH=%PATH%;C:\Program Files\7-Zip
set ROOT_DIR=%~dp0..\
set CONFIGURATION=Release
if '%1'=='--debug' (
    set CONFIGURATION=Debug
)

set COMPONENT_DIR_NO_SLASH=%ROOT_DIR%component
set RESULT_CONFIGURATION_DIR=%ROOT_DIR%_result\Win32_%CONFIGURATION%\
set COMPONENT_DLL=%RESULT_CONFIGURATION_DIR%\bin\foo_spider_monkey_panel.dll
set SAMPLES_COMPLETE_DIR_NO_SLASH=%ROOT_DIR%submodules\js_marc2003_complete
set MOZ_JS_BIN_DIR=%ROOT_DIR%mozjs\%CONFIGURATION%\bin\
set COMPONENT_OUT_DIR_NO_SLASH=%RESULT_CONFIGURATION_DIR%\component
set COMPONENT_OUT_DIR=%COMPONENT_OUT_DIR_NO_SLASH%\
set FB2K_ARCHIVE=%RESULT_CONFIGURATION_DIR%foo_spider_monkey_panel.fb2k-component

@echo on

if not exist "%COMPONENT_OUT_DIR_NO_SLASH%" mkdir "%COMPONENT_OUT_DIR_NO_SLASH%"
xcopy /r/y/s "%COMPONENT_DIR_NO_SLASH%" "%COMPONENT_OUT_DIR_NO_SLASH%"
echo d|xcopy /r/y/s "%SAMPLES_COMPLETE_DIR_NO_SLASH%" "%COMPONENT_OUT_DIR_NO_SLASH%\samples\complete"
xcopy /r/y/s "%MOZ_JS_BIN_DIR%*.dll" "%COMPONENT_OUT_DIR%"
xcopy /r/y "%COMPONENT_DLL%" "%COMPONENT_OUT_DIR%"

if exist "%FB2K_ARCHIVE%" del /f/q "%FB2K_ARCHIVE%"
7z a -tzip "%FB2K_ARCHIVE%" "%COMPONENT_OUT_DIR%*"
