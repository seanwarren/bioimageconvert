@ECHO OFF
rem @SET MODE=x86
SET MODE=x64
if not "%1" == "" @SET MODE=%1

echo.
if "%MODE%"=="x64" goto PresetX64

:PresetX86
echo Setting environment for MinGW x32

SET MINGWDIR=c:\Programs\Develop\mingw64

set PATH=%MINGWDIR%\bin;%PATH%
set INCLUDE=%MINGWDIR%\INCLUDE%INCLUDE%
set LIB=%MINGWDIR%\lib;%LIB%
set LIBPATH=%MINGWDIR%\LIB;%LIBPATH%

goto CreateLibs

:PresetX64
echo Setting environment for MinGW x64
SET MINGWDIR=c:\Programs\Develop\mingw64

set PATH=%MINGWDIR%\bin;%PATH%
set INCLUDE=%MINGWDIR%\INCLUDE%INCLUDE%
set LIB=%MINGWDIR%\lib;%LIB%
set LIBPATH=%MINGWDIR%\LIB;%LIBPATH%

rem @goto CreateLibs


:CreateLibs
echo.
cd ffmpeg
make all install
echo.