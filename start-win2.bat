@echo off

set PATH=%PATH%;C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x64;C:\usr\lib\opensg;C:\usr\lib\collada;C:\usr\vcpkg\installed\x64-windows\lib;C:\usr\lib\cef;C:\usr\lib\oce;C:\usr\lib\ifc;C:\usr\lib\openvr;
set PATH=%PATH%;D:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x64;D:\usr\lib\opensg;D:\usr\lib\collada;D:\usr\vcpkg\installed\x64-windows\lib;D:\usr\lib\cef;D:\usr\lib\oce;D:\usr\lib\ifc;C:\usr\lib\openvr;
set PYTHONPATH=%PYTHONPATH%;C:\usr\vcpkg\installed\x64-windows\share\python2\Lib
set PYTHONPATH=%PYTHONPATH%;D:\usr\vcpkg\installed\x64-windows\share\python2\Lib
set DISPLAY=
rem TODO: fix gschemas path!
rem  set XDG_DATA_DIR=%XDG_DATA_DIR%;ressources\gui\schemas

echo "start polyvr.exe"
build\Release\polyvr.exe %*
rem "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv" /debugexe build\RelWithDebInfo\polyvr.exe %*