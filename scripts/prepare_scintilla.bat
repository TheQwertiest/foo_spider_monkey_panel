@echo off
setlocal

set CUR_DIR=%~dp0
set ROOT_DIR=%1
if '%ROOT_DIR%'=='' set ROOT_DIR=%CUR_DIR%..\
set SCINTILLA_DIR=%ROOT_DIR%scintilla\

rem Remove extra lexers
for %%i in ("%SCINTILLA_DIR%lexers\"*) do if not %%~nxi == LexCPP.cxx del /q "%%i"

cd %SCINTILLA_DIR%scripts
@echo on
python LexGen.py
xcopy /r/y "%CUR_DIR%additional_files\scintilla.vcxproj" "%SCINTILLA_DIR%win32\"

