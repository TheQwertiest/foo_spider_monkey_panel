@echo off
setlocal

rem Parse repo name instead of using hard coded value
call py generate_source_link.py
if errorlevel 1 goto fail

echo Source link configuration file was successfully generated!
exit /b 0

:fail
echo Source link configuration file failed!
exit /b 1
