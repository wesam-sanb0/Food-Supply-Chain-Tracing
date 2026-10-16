@echo off
rem run_all.bat - Role 3: compile with -O2 and run every standard experiment (Windows)
rem
rem Usage, from the repository root (Command Prompt):
rem     scripts\run_all.bat "Your Name"
rem
rem Output: results\<Your_Name>.csv
rem Before running: laptop plugged in, other apps closed.
rem Needs g++ (MinGW-w64) or clang++ on the PATH.

setlocal EnableExtensions
cd /d "%~dp0.."

set "MEMBER=%~1"
if "%MEMBER%"=="" set /p "MEMBER=Your name in English (written to the CSV): "
if "%MEMBER%"=="" (
    echo Error: a name is required.
    goto :fail
)

if "%RUNS%"=="" set "RUNS=10"
set "SEED=42"
set "DEG=3"

rem ---- compiler -------------------------------------------------------------
set "CXX="
where g++ >nul 2>nul
if not errorlevel 1 set "CXX=g++"
if not defined CXX (
    where clang++ >nul 2>nul
    if not errorlevel 1 set "CXX=clang++"
)
if not defined CXX (
    echo Error: no g++ or clang++ found on the PATH. Install MinGW-w64, see the README.
    goto :fail
)

rem ---- CPU model --------------------------------------------------------------
set "CPU="
for /f "usebackq delims=" %%i in (`powershell -NoProfile -Command "(Get-CimInstance Win32_Processor | Select-Object -First 1).Name.Trim()"`) do set "CPU=%%i"
if not defined CPU set "CPU=unknown"

rem ---- output file ------------------------------------------------------------
set "SAFE_NAME=%MEMBER: =_%"
if not exist results mkdir results
set "OUT=results\%SAFE_NAME%.csv"
if exist "%OUT%" del "%OUT%"

echo Member : %MEMBER%
echo CPU    : %CPU%
echo Output : %OUT%
echo Compiling with %CXX% -O2 ...
%CXX% -std=c++17 -O2 -Isrc src\main.cpp src\generator.cpp src\trace.cpp -o food_trace.exe
if errorlevel 1 (
    echo Error: compilation failed.
    goto :fail
)

rem Default sizes, 1,000 reported batches
call :run 10000 1000   || goto :fail
call :run 100000 1000  || goto :fail
call :run 500000 1000  || goto :fail
call :run 1000000 1000 || goto :fail
rem Extra test on the 100K graph: vary the number of reported batches
call :run 100000 100   || goto :fail
call :run 100000 10000 || goto :fail

echo.
echo Done. Results saved to %OUT%
exit /b 0

:run
food_trace.exe --nodes %1 --reported %2 --seed %SEED% --deg %DEG% --runs %RUNS% --member "%MEMBER%" --cpu "%CPU%" --out "%OUT%"
exit /b %errorlevel%

:fail
exit /b 1
