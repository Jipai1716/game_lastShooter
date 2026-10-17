@echo off
setlocal enabledelayedexpansion
rem ===========================================================================
rem  Last Shooter - build script (MinGW-w64 + EasyX)
rem
rem  Usage:  build.bat            build Release
rem          build.bat debug      build with debug symbols, console visible
rem          build.bat run        build then run
rem
rem  Requires a MinGW-w64 g++ (x64 or x86). EasyX is bundled in third_party/easyx.
rem  The script auto-detects a few common toolchain locations.
rem ===========================================================================

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "MODE=release"
set "RUN="
for %%a in (%*) do (
  if /I "%%a"=="debug" set "MODE=debug"
  if /I "%%a"=="run"   set "RUN=1"
)

rem ---------------------------------------------------------------- find g++ --
set "GXX="
if not "%LS_GXX%"=="" if exist "%LS_GXX%" set "GXX=%LS_GXX%"
if "%GXX%"=="" (
  for %%p in (
    "D:\program\RedPanda-Cpp\MinGW64\bin\g++.exe"
    "C:\Program Files\RedPanda-Cpp\MinGW64\bin\g++.exe"
    "C:\Program Files\Dev-Cpp\MinGW64\bin\g++.exe"
    "C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe"
    "C:\Dev-Cpp\MinGW64\bin\g++.exe"
    "D:\program\DEV_CPP\DEV_CPP\Dev-Cpp\MinGW64\bin\g++.exe"
    "C:\msys64\mingw64\bin\g++.exe"
    "C:\mingw64\bin\g++.exe"
  ) do (
    if not defined GXX if exist "%%~p" set "GXX=%%~p"
  )
)
if "%GXX%"=="" (
  for /f "delims=" %%i in ('where g++ 2^>nul') do if not defined GXX set "GXX=%%i"
)
if "%GXX%"=="" (
  echo [build] ERROR: g++ not found. Install MinGW-w64 or set LS_GXX to g++.exe
  echo         e.g.  set LS_GXX=C:\mingw64\bin\g++.exe
  exit /b 1
)

for %%i in ("%GXX%") do set "GXXDIR=%%~dpi"
set "PATH=%GXXDIR%;%PATH%"
echo [build] g++       : %GXX%

rem ------------------------------------------------------- target arch/lib --
set "TRIPLE="
for /f "delims=" %%i in ('"%GXX%" -dumpmachine') do set "TRIPLE=%%i"
set "ARCH=x86_64"
set "LIBDIR=lib_mingw_msvcrt\x64"
echo %TRIPLE% | findstr /I "x86_64" >nul
if errorlevel 1 (
  set "ARCH=i686"
  set "LIBDIR=lib_mingw_msvcrt\x86"
)
echo [build] target    : %TRIPLE% (%ARCH%)

rem Dev-C++ MinGW 4.9.2 cannot compile C++17: fall back to C++11 with a warning.
set "STD=-std=c++17"
set "MINGW_VER="
for /f "tokens=* delims=" %%i in ('"%GXX%" -dumpversion') do set "MINGW_VER=%%i"
for /f "tokens=1 delims=." %%i in ("%MINGW_VER%") do set "MAJOR=%%i"
if %MAJOR% LSS 7 (
  echo [build] WARNING: g++ %MINGW_VER% is old, switching to -std=c++14
  set "STD=-std=c++14"
)

rem ------------------------------------------------------------------ flags --
set "CXXFLAGS=%STD% -O2 -fno-strict-aliasing -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers"
set "CXXFLAGS=%CXXFLAGS% -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX"
set "CXXFLAGS=%CXXFLAGS% -I"%ROOT%\third_party\easyx\include" -I"%ROOT%\src""
set "LDFLAGS=-static -static-libgcc -static-libstdc++ -mwindows"
set "LIBS=-L"%ROOT%\third_party\easyx\%LIBDIR%" -leasyxw -lwinmm -lgdi32 -lole32 -luuid -lmsimg32 -lgdiplus -luser32 -lshell32 -lshlwapi"
if /I "%MODE%"=="debug" (
  set "CXXFLAGS=-g -O0 -DLS_DEBUG %STD% -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX -I"%ROOT%\third_party\easyx\include" -I"%ROOT%\src""
  set "LDFLAGS=-static -static-libgcc -static-libstdc++"
)
if /I exist "%ROOT%\build" goto :have_build
mkdir "%ROOT%\build"
:have_build

rem -------------------------------------------------------------- collect src --
set "SOURCES="
for /r "%ROOT%\src" %%f in (*.cpp) do set "SOURCES=!SOURCES! "%%f""
if "%SOURCES%"=="" (
  echo [build] ERROR: no .cpp files under src\
  exit /b 1
)

set "OUT=%ROOT%\build\lastShooter.exe"
echo [build] mode      : %MODE%
echo [build] output    : %OUT%
echo [build] compiling...

del /q "%ROOT%\build\lastShooter.exe" 2>nul
"%GXX%" %CXXFLAGS% %SOURCES% -o "%OUT%" %LIBS% %LDFLAGS% 2> "%ROOT%\build\build.log"
if errorlevel 1 (
  echo [build] FAILED. compiler output:
  echo ---------------------------------------------------------------
  type "%ROOT%\build\build.log"
  echo ---------------------------------------------------------------
  exit /b 1
)
for %%i in ("%OUT%") do set "SIZE=%%~zi"
echo [build] OK -^> %OUT%  (!SIZE! bytes)

if defined RUN (
  pushd "%ROOT%"
  "%OUT%"
  set "RC=%ERRORLEVEL%"
  popd
  exit /b %RC%
)
endlocal
