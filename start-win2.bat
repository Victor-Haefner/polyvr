@echo off

set PATH=%PATH%;C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x64;C:\usr\lib\opensg;C:\usr\lib\collada;C:\usr\vcpkg\installed\x64-windows\lib;C:\usr\lib\cef;C:\usr\lib\oce;
set PATH=%PATH%;D:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x64;D:\usr\lib\opensg;D:\usr\lib\collada;D:\usr\vcpkg\installed\x64-windows\lib;D:\usr\lib\cef;D:\usr\lib\oce;
set PYTHONPATH=%PYTHONPATH%;C:\usr\vcpkg\installed\x64-windows\share\python2\Lib
set PYTHONPATH=%PYTHONPATH%;D:\usr\vcpkg\installed\x64-windows\share\python2\Lib
set DISPLAY=
rem TODO: fix gschemas path!
rem  set XDG_DATA_DIR=%XDG_DATA_DIR%;ressources\gui\schemas
build\Release\polyvr.exe %*