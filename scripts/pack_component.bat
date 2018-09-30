@echo off
setlocal

set PATH=%PATH%;C:\Program Files\7-Zip

set ROOT_DIR=%~dp0..\
if not '%1'=='' if not '%1'=='--debug' (
    set ROOT_DIR=%1
)

set CONFIGURATION=Release
if '%1'=='--debug' (
    set CONFIGURATION=Debug
)
if '%2'=='--debug' (
    set CONFIGURATION=Debug
)

set COMPONENT_DIR_NO_SLASH=%ROOT_DIR%component
set RESULT_CONFIGURATION_DIR=%ROOT_DIR%_result\Win32_%CONFIGURATION%\
set COMPONENT_DLL=%RESULT_CONFIGURATION_DIR%\bin\foo_spider_monkey_panel.dll
set SAMPLES_COMPLETE_DIR_NO_SLASH=%ROOT_DIR%submodules\smp_2003
set MOZ_JS_BIN_DIR=%ROOT_DIR%mozjs\%CONFIGURATION%\bin\
set COMPONENT_OUT_DIR_NO_SLASH=%RESULT_CONFIGURATION_DIR%component
set COMPONENT_OUT_DIR=%COMPONENT_OUT_DIR_NO_SLASH%\
set FB2K_ARCHIVE=%RESULT_CONFIGURATION_DIR%foo_spider_monkey_panel.fb2k-component

echo Packing component to .fb2k-component

if not exist "%COMPONENT_OUT_DIR_NO_SLASH%" mkdir "%COMPONENT_OUT_DIR_NO_SLASH%"
xcopy /r/y/s/q/i "%COMPONENT_DIR_NO_SLASH%" "%COMPONENT_OUT_DIR_NO_SLASH%"
if errorlevel 1 goto fail
xcopy /r/y/s/q/i "%SAMPLES_COMPLETE_DIR_NO_SLASH%" "%COMPONENT_OUT_DIR_NO_SLASH%\samples\complete"
if errorlevel 1 goto fail
xcopy /r/y/s/q "%MOZ_JS_BIN_DIR%*.dll" "%COMPONENT_OUT_DIR%"
if errorlevel 1 goto fail
xcopy /r/y/q "%COMPONENT_DLL%" "%COMPONENT_OUT_DIR%"
if errorlevel 1 goto fail

if exist "%FB2K_ARCHIVE%" del /f/q "%FB2K_ARCHIVE%"
7z a -tzip "%FB2K_ARCHIVE%" "%COMPONENT_OUT_DIR%*" > NUL
echo Component was sucessfuly packed: %FB2K_ARCHIVE%
if errorlevel 1 goto fail
exit /b 0

:fail
echo Failed!
exit /b 1
