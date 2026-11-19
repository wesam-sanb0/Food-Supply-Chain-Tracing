@echo off
rem run_parallel.bat - Assignment 2 (Jory Altwijry): compile with OpenMP and run every
rem version on 1, 2, 4 and 8 threads (Windows)
rem
rem Usage, from the repository root (Command Prompt):
rem     scripts\run_parallel.bat "Your Name"
rem
rem Output: results\parallel\<Your_Name>.csv
rem Before running: laptop plugged in, other apps closed.
rem Needs g++ (MinGW-w64) on the PATH; it includes OpenMP (-fopenmp).
rem
rem Optional: set RUNS=5 before running for fewer timed runs (default 10),
rem           set THREADS=1 2 4 for other thread counts (default 1 2 4 8).

setlocal EnableExtensions
cd /d "%~dp0.."

set "MEMBER=%~1"
if "%MEMBER%"=="" set /p "MEMBER=Your name in English (written to the CSV): "
if "%MEMBER%"=="" (
    echo Error: a name is required.
    goto :fail
)

if "%RUNS%"=="" set "RUNS=10"
if "%THREADS%"=="" set "THREADS=1 2 4 8"
set "VERSIONS=race critical atomic reduction"
set "SEED=42"
set "DEG=3"

rem ---- compiler -------------------------------------------------------------
where g++ >nul 2>nul
if errorlevel 1 (
    echo Error: g++ not found on the PATH. Install MinGW-w64, see the README.
    goto :fail
)
set "CXX=g++"

rem ---- CPU model --------------------------------------------------------------
set "CPU="
for /f "usebackq delims=" %%i in (`powershell -NoProfile -Command "(Get-CimInstance Win32_Processor | Select-Object -First 1).Name.Trim()"`) do set "CPU=%%i"
if not defined CPU set "CPU=unknown"

rem ---- output file ------------------------------------------------------------
set "SAFE_NAME=%MEMBER: =_%"
if not exist results\parallel mkdir results\parallel
set "OUT=results\parallel\%SAFE_NAME%.csv"
if exist "%OUT%" del "%OUT%"

echo Member  : %MEMBER%
echo CPU     : %CPU%  (%NUMBER_OF_PROCESSORS% logical CPUs)
echo Threads : %THREADS%
echo Output  : %OUT%
echo Compiling with %CXX% -O2 -fopenmp ...
%CXX% -std=c++17 -O2 -fopenmp -Isrc src\main.cpp src\generator.cpp src\trace.cpp src\parallel_trace.cpp src\parallel_sync.cpp -o food_trace_omp.exe
if errorlevel 1 (
    echo Error: compilation failed.
    goto :fail
)

rem Three workloads: the Stage 1 reference size, the largest graph,
rem and the heaviest tracing load (most reported batches).
call :workload 100000 1000   || goto :fail
call :workload 1000000 1000  || goto :fail
call :workload 100000 10000  || goto :fail

echo.
echo Done. Results saved to %OUT%
exit /b 0

:workload
call :run %1 %2 seq 1 || exit /b 1
for %%V in (%VERSIONS%) do (
    for %%T in (%THREADS%) do (
        call :run %1 %2 %%V %%T || exit /b 1
    )
)
exit /b 0

:run
food_trace_omp.exe --nodes %1 --reported %2 --seed %SEED% --deg %DEG% --runs %RUNS% --version %3 --threads %4 --member "%MEMBER%" --cpu "%CPU%" --out "%OUT%"
exit /b %errorlevel%

:fail
exit /b 1
