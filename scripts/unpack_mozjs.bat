@echo off
setlocal

set PATH=%PATH%;C:\Program Files\7-Zip
set ROOT_DIR=%~dp0..\

set MOZ_JS_IN_DIR=%ROOT_DIR%submodules\mozjs\
set MOZ_JS_OUT_DIR_NO_SLASH=%ROOT_DIR%mozjs

echo Unpacking mozjs files

if exist "%MOZ_JS_OUT_DIR_NO_SLASH%" rmdir /s/q "%MOZ_JS_OUT_DIR_NO_SLASH%"
mkdir "%MOZ_JS_OUT_DIR_NO_SLASH%"

for /f "delims=" %%i in ('dir "%MOZ_JS_IN_DIR%"* /b ^| findstr /r/i "mozjs.*\.zip"') do (
    7z x -tzip -o"%MOZ_JS_OUT_DIR_NO_SLASH%" "%MOZ_JS_IN_DIR%%%i" > NUL
    if errorlevel 1 goto fail
    goto success
)
:fail
echo Failed!
exit /b 1

:success
exit /b 0