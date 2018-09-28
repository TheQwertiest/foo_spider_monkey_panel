@echo off
setlocal

set PATH=%PATH%;C:\Program Files\7-Zip
set ROOT_DIR=%~dp0..\

set MOZ_JS_DIR=%ROOT_DIR%mozjs\

echo Unpacking mozjs files

for /d %%i in ("%MOZ_JS_DIR%"*) do rmdir /s /q "%%i"
for /f "delims=" %%i in ('dir "%MOZ_JS_DIR%"* /b ^| findstr /r/i "mozjs.*\.zip"') do (
    7z x -tzip -o"%MOZ_JS_DIR%" "%MOZ_JS_DIR%%%i" > NUL
    if errorlevel 1 goto fail
    goto success
)
:fail
echo Failed!
exit /b 1

:success
exit /b 0