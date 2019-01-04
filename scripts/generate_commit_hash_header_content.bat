@echo off
setlocal

echo #pragma once
echo.
echo #ifdef SMP_COMMIT_HASH
echo #    undef SMP_COMMIT_HASH
echo #endif
echo #define SMP_COMMIT_HASH \

call git rev-parse --short HEAD
if errorlevel 1 goto fail

exit /b 0

:fail
exit /b 1