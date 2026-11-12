@echo off
rem Windows: build the game from your disc (put the ISO in imports\psp\ first). No PowerShell needed.
rem Options: build.cmd --help   (details: docs\BUILDING.md); --clean starts from nothing (keeps your saves)
setlocal
cd /d "%~dp0"
rem --clean: .tools\ goes first, here, since the Python below may run from it (build.py deletes the rest).
rem Kept: librashader (built with Rust: slow to make again, and maybe built by hand), its source
rem folders (librashader-*) and the FILTER shaders. --clean-all deletes those too.
set "CLEAN="
set "CLEAN_ALL="
for %%A in (%*) do if /i "%%~A"=="--clean" set "CLEAN=1"
for %%A in (%*) do if /i "%%~A"=="--clean-all" set "CLEAN=1" & set "CLEAN_ALL=1"
if defined CLEAN if exist ".tools" (
    if defined CLEAN_ALL (echo ==^> Clean: deleting .tools) else (echo ==^> Clean: deleting .tools, keeping librashader and the FILTER shaders ^(--clean-all deletes them too^))
    set "CLEAN_FAILED="
    for /d %%D in (".tools\*") do call :cleanone "%%~nxD"
    del /q ".tools\*" 2>nul
    if defined CLEAN_ALL rmdir ".tools" 2>nul
    call :cleancheck
    if errorlevel 1 (
        echo FAILED: could not delete everything in .tools. Close the game, the launcher and anything else using that folder, then try again.
        if "%~1"=="" pause
        exit /b 1
    )
)
rem Python's embeddable package, pinned: the same URL and SHA-256 as tools\data\pins.json (tools\pins.py --check)
set "PY_URL=https://www.python.org/ftp/python/3.12.10/python-3.12.10-embed-amd64.zip"
set "PY_SHA256=4acbed6dd1c744b0376e3b1cf57ce906f9dc9e95e68824584c8099a63025a3c3"
set "PY="
rem a Python 3.9+ of your own that can verify HTTPS certificates, else ours in .tools\python.
rem (The Microsoft Store placeholder fails this test, and so do Pythons that other programs bring
rem with no certificate store, such as Inkscape's: issue #52. A3MX_TEST_NO_CERTS=1 fails it on purpose.)
python -c "import os, sys, ssl; sys.exit(0 if sys.version_info >= (3, 9) and ssl.create_default_context().cert_store_stats()['x509_ca'] and not os.environ.get('A3MX_TEST_NO_CERTS') else 1)" >nul 2>nul && set "PY=python"
if not defined PY if exist ".tools\python\python.exe" set "PY=.tools\python\python.exe"
if not defined PY (
    call :getpython
    if errorlevel 2 goto :badhash
    if errorlevel 1 goto :nodownload
    set "PY=.tools\python\python.exe"
)
"%PY%" tools\build.py %*
set "RC=%ERRORLEVEL%"
if "%~1"=="" pause
exit /b %RC%

rem --clean: one folder of .tools\, unless it is one that is kept
:cleanone
set "N=%~1"
if not defined CLEAN_ALL (
    if /i "%N%"=="librashader" exit /b 0
    if /i "%N%"=="slang-shaders" exit /b 0
    if /i "%N:~0,12%"=="librashader-" exit /b 0
)
rmdir /s /q ".tools\%N%" 2>nul
if exist ".tools\%N%" set "CLEAN_FAILED=1"
exit /b 0
:cleancheck
if defined CLEAN_FAILED exit /b 1
exit /b 0

rem Python's embeddable package, checked against its SHA-256 before it is unpacked (certutil comes with Windows)
:getpython
echo ==^> Python: downloading into .tools\python
if not exist ".tools" mkdir ".tools"
curl.exe -fsSL --retry 3 -o ".tools\python.zip" "%PY_URL%" || exit /b 1
set "GOT="
for /f "skip=1 delims=" %%H in ('certutil -hashfile ".tools\python.zip" SHA256') do if not defined GOT set "GOT=%%H"
if defined GOT set "GOT=%GOT: =%"
if /i not "%GOT%"=="%PY_SHA256%" (
    del ".tools\python.zip" 2>nul
    exit /b 2
)
echo   SHA-256 checked
mkdir ".tools\python" 2>nul
tar.exe -xf ".tools\python.zip" -C ".tools\python" || exit /b 1
del ".tools\python.zip"
rem a normal sys.path, so tools\ can import its own modules
del ".tools\python\*._pth" 2>nul
exit /b 0

:badhash
echo FAILED: the Python download does not match its SHA-256 (got %GOT%, expected %PY_SHA256%). Try again; if it happens again, report it.
if "%~1"=="" pause
exit /b 1
:nodownload
echo FAILED: could not download Python. Install Python 3.9+ from https://www.python.org and run this again.
if "%~1"=="" pause
exit /b 1
