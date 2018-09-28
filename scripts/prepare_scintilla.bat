@echo off
setlocal

set CUR_DIR=%~dp0
set ROOT_DIR=%1
if '%ROOT_DIR%'=='' set ROOT_DIR=%CUR_DIR%..\
set SCINTILLA_DIR=%ROOT_DIR%submodules\scintilla\

echo Configuring scintilla

rem Remove extra lexers
for %%i in ("%SCINTILLA_DIR%lexers\"*) do if not %%~nxi == LexCPP.cxx del /q "%%i"

cd %SCINTILLA_DIR%scripts

python LexGen.py
if errorlevel 1 goto fail
xcopy /r/y/q "%CUR_DIR%additional_files\scintilla.vcxproj" "%SCINTILLA_DIR%win32\"
if errorlevel 1 goto fail
exit /b 0

:fail
echo Failed!
exit /b 1
