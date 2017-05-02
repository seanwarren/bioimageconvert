@ECHO OFF
rem @SET MODE=x86
SET MODE=x64
if not "%1" == "" @SET MODE=%1

echo.
if "%MODE%"=="x64" goto PresetX64

:PresetX86
rem set PATH=C:\Program Files\Microsoft Visual Studio 9.0\VC\bin;%PATH%
rem set PATH=C:\Program Files\Microsoft Visual Studio 9.0\Common7\IDE;%PATH%
SET VSINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 9.0
SET VCINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC

set PATH=%VCINSTALLDIR%\BIN;%VCINSTALLDIR%\VCPackages;%VSINSTALLDIR%\Common7\IDE;%VSINSTALLDIR%\Common7\Tools;%VSINSTALLDIR%\Common7\Tools\bin;%PATH%
set INCLUDE=%VCINSTALLDIR%\ATLMFC\INCLUDE;%VCINSTALLDIR%\INCLUDE;%INCLUDE%
set LIB=%VCINSTALLDIR%\ATLMFC\LIB;%VCINSTALLDIR%\LIB;%LIB%
set LIBPATH=%VCINSTALLDIR%\ATLMFC\LIB;%VCINSTALLDIR%\LIB;%LIBPATH%
set LIBMACHINE=X86

echo Setting environment for Visual Studio 2008 x86
goto CreateLibs

:PresetX64
SET VSINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 9.0
SET VCINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC

set PATH=%VCINSTALLDIR%\BIN\amd64;%VCINSTALLDIR%\VCPackages;%VSINSTALLDIR%\Common7\IDE;%VSINSTALLDIR%\Common7\Tools;%VSINSTALLDIR%\Common7\Tools\bin;%PATH%
set INCLUDE=%VCINSTALLDIR%\ATLMFC\INCLUDE;%VCINSTALLDIR%\INCLUDE;%INCLUDE%
set LIB=%VCINSTALLDIR%\ATLMFC\LIB\amd64;%VCINSTALLDIR%\LIB\amd64;%LIB%
set LIBPATH=%VCINSTALLDIR%\ATLMFC\LIB\amd64;%VCINSTALLDIR%\LIB\amd64;%LIBPATH%
set LIBMACHINE=X64

echo Setting environment for Visual Studio 2008 x64
rem @goto CreateLibs


:CreateLibs
echo.
lib /nologo /machine:%LIBMACHINE% /def:avutil-50.def /name:avutil-50.dll
lib /nologo /machine:%LIBMACHINE% /def:avformat-52.def /name:avformat-52.dll
lib /nologo /machine:%LIBMACHINE% /def:avcodec-52.def /name:avcodec-52.dll
lib /nologo /machine:%LIBMACHINE% /def:swscale-0.def /name:swscale-0.dll
del *.exp

rem copy *.dll ..\..\..\libs\vc2008
rem copy *.lib ..\..\..\libs\vc2008
echo.