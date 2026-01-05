@echo off

if not exist src (
    echo Please run this script from the project root directory - 'src' not found.
    pause
    exit /b 1
)
if not exist tests\tmp mkdir tests\tmp

echo.
echo Building tests...

gcc -Iinclude -g -O0 -o tests\test_binio.exe tests\test_binio.c src\binio.c src\loglib.c
if errorlevel 1 goto build_fail

gcc -Iinclude -g -O0 -o tests\test_loglib.exe tests\test_loglib.c src\loglib.c
if errorlevel 1 goto build_fail

gcc -Iinclude -g -O0 -o tests\test_savesdir.exe tests\test_savesdir.c src\savesdir.c src\binio.c src\loglib.c -lpsapi
if errorlevel 1 goto build_fail

echo Successfully built tests.

echo.
echo Running tests...

echo.
echo Running test_binio.exe
tests\test_binio.exe || goto run_fail

echo.
echo Running test_loglib.exe
tests\test_loglib.exe || goto run_fail

echo.
echo Running test_savesdir.exe
tests\test_savesdir.exe || goto run_fail

echo.
echo All tests finished.
exit /b 0

:build_fail
    echo.
    echo Build failed.
    pause
    exit /b 1

:run_fail
    echo.
    echo Test failed (non-zero exit). See output above for details.
    pause
    exit /b 2
