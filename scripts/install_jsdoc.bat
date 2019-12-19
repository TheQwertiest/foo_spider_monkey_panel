@echo off
setlocal

echo Installing npm packages

call npm install -g jsdoc tui-jsdoc-template
if errorlevel 1 goto fail

echo Installation complete!
exit /b 0

:fail
echo Failed!
exit /b 1
