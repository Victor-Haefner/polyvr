@echo off
set PATH=%PATH%;C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x64;C:\usr\lib\opensg;C:\usr\vcpkg\installed\x64-windows\lib;
set PATH=%PATH%;D:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x64;D:\usr\lib\opensg;D:\usr\vcpkg\installed\x64-windows\lib;
set DISPLAY=
cd %~dp0
start /min VRServer.exe %*
rem start /min VRServer.exe -w -m L1 -geometry 512x512+0+0
rem PsExec64.exe -accepteula -i -d -s \\\\127.0.0.1 CMD /C "start VRServer.exe -w -m L1 -geometry 512x512+0+0"