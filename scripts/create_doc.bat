@echo off
setlocal

set CUR_DIR=%~dp0
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
set COMPONENT_OUT_DIR_NO_SLASH=%RESULT_CONFIGURATION_DIR%component
set COMPONENT_OUT_DIR=%COMPONENT_OUT_DIR_NO_SLASH%\

set SRC_JS=%COMPONENT_DIR_NO_SLASH%\docs\js\foo_spider_monkey_panel.js
set DOC_DIR_NO_SLASH=%ROOT_DIR%_result\html


echo Creating HTML doc

if exist "%DOC_DIR_NO_SLASH%" rmdir /s/q "%DOC_DIR_NO_SLASH%"
mkdir "%DOC_DIR_NO_SLASH%"

call jsdoc -c %CUR_DIR%doc/conf.json --readme %CUR_DIR%doc/README.md %SRC_JS% -d %DOC_DIR_NO_SLASH%
if errorlevel 1 goto fail

echo Doc was sucessfuly created: %DOC_DIR_NO_SLASH%
exit /b 0

:fail
echo Failed!
exit /b 1
