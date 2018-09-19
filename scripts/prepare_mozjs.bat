@echo off
setlocal

set PATH=%PATH%;C:\Program Files\7-Zip
set ROOT_DIR=%~dp0..\

set MOZ_JS_DIR=%ROOT_DIR%mozjs\
set MOZ_JS_ARCHIVE_NAME=mozjs_temp.zip
set MOZ_JS_ARCHIVE=%MOZ_JS_DIR%%MOZ_JS_ARCHIVE_NAME%

for /d %%i in ("%MOZ_JS_DIR%"*) do rmdir /s /q "%%i"

@echo on
7z x -tzip -o"%MOZ_JS_DIR%" "%MOZ_JS_ARCHIVE%"
