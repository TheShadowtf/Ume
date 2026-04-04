@echo off
setlocal

set "ROOT=%~dp0"
set "BUILD_DIR=%ROOT%build-cmake"
set "TARGET=UmeLauncher"
set "CONFIG=Debug"

if not "%~1"=="" set "TARGET=%~1"
if not "%~2"=="" set "CONFIG=%~2"

if /I not "%TARGET%"=="UmeLauncher" if /I not "%TARGET%"=="UmeEditor" if /I not "%TARGET%"=="Sandbox" (
    echo [ERROR] Target must be UmeLauncher, UmeEditor or Sandbox.
    exit /b 1
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake was not found in PATH.
    exit /b 1
)

echo [1/3] Configuring CMake (%CONFIG%)...
cmake -S "%ROOT%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1

echo [2/3] Building %TARGET% (%CONFIG%)...
cmake --build "%BUILD_DIR%" --config "%CONFIG%" --target %TARGET%
if errorlevel 1 exit /b 1

set "EXE=%BUILD_DIR%\bin\%CONFIG%\%TARGET%.exe"
if not exist "%EXE%" (
    echo [ERROR] Could not find executable: %EXE%
    exit /b 1
)

echo [3/3] Running %TARGET%...
if /I "%TARGET%"=="Sandbox" (
    pushd "%ROOT%Sandbox"
) else if /I "%TARGET%"=="UmeEditor" (
    pushd "%ROOT%UmeEditor"
) else (
    pushd "%ROOT%UmeLauncher"
)
"%EXE%"
popd

endlocal
