@ECHO OFF
rem @SET MODE=x86
SET MODE=x64
if not "%1" == "" @SET MODE=%1

echo.
if "%MODE%"=="x64" goto PresetX64

:PresetX86
echo Setting environment for Visual Studio 2010 x86

SET VSINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 12.0
SET VCINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC
SET VCBUILDDIR=C:\Program Files (x86)\MSBuild\12.0\bin

set PATH=%VSINSTALLDIR%\Common7\Tools;%PATH%
set PATH=%VSINSTALLDIR%\Common7\IDE;%PATH%
set PATH=%VCINSTALLDIR%\VCPackages;%PATH%
set PATH=%VCINSTALLDIR%\bin\x86_amd64;%PATH%
set PATH=%VCBUILDDIR%;%PATH%

set INCLUDE=%VCINSTALLDIR%\ATLMFC\INCLUDE;%VCINSTALLDIR%\INCLUDE;%INCLUDE%
set LIB=%VCINSTALLDIR%\ATLMFC\LIB;%VCINSTALLDIR%\LIB;%LIB%
set LIBPATH=%VCINSTALLDIR%\ATLMFC\LIB;%VCINSTALLDIR%\LIB;%LIBPATH%
set LIBMACHINE=X86

goto CreateLibs

:PresetX64
echo Setting environment for Visual Studio 2013 x64

SET VSINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 12.0
SET VCINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC
SET VCBUILDDIR=C:\Program Files (x86)\MSBuild\12.0\bin\amd64

set PATH=%VSINSTALLDIR%\Common7\Tools;%PATH%
set PATH=%VSINSTALLDIR%\Common7\IDE;%PATH%
set PATH=%VCINSTALLDIR%\VCPackages;%PATH%
set PATH=%VCINSTALLDIR%\bin\amd64;%PATH%
set PATH=%VCBUILDDIR%;%PATH%

set INCLUDE=%VCINSTALLDIR%\ATLMFC\INCLUDE;%VCINSTALLDIR%\INCLUDE;%INCLUDE%
set LIB=%VCINSTALLDIR%\ATLMFC\LIB\amd64;%VCINSTALLDIR%\LIB\amd64;%LIB%
set LIBPATH=%VCINSTALLDIR%\ATLMFC\LIB\amd64;%VCINSTALLDIR%\LIB\amd64;%LIBPATH%
set LIBMACHINE=X64

rem @goto CreateLibs


:CreateLibs
echo.
rem this is not yet enough, file producesd should be stripped and appended with: EXPORTS
rem dumpbin /exports avcodec-52.dll > avcodec-52.def
lib /nologo /machine:%LIBMACHINE% /def:avutil-52.def /name:avutil-52.dll
lib /nologo /machine:%LIBMACHINE% /def:avformat-55.def /name:avformat-55.dll
lib /nologo /machine:%LIBMACHINE% /def:avcodec-55.def /name:avcodec-55.dll
lib /nologo /machine:%LIBMACHINE% /def:swscale-2.def /name:swscale-2.dll
del *.exp

rem copy *.dll ..\..\..\libs\vc2008
rem copy *.lib ..\..\..\libs\vc2008
echo.