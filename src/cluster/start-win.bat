@echo off

rem example:
rem  start-win.bat -w -m L1 -geometry 512x512+0+0

set DIR=%~dp0
set PATH=%PATH%;C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x64;C:\usr\lib\opensg;C:\usr\vcpkg\installed\x64-windows\lib;
set PATH=%PATH%;D:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x64;D:\usr\lib\opensg;D:\usr\vcpkg\installed\x64-windows\lib;
set PATH=%PATH%;%DIR%..\..\libs;
set DISPLAY=
cd %DIR%
start /min VRServer.exe %*
