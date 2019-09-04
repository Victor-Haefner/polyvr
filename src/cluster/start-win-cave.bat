cd %~dp0
set PATH=%PATH%;C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x64;C:\opensg;C:\freeglut;C:\boost\lib64-msvc-14.1;C:\zlib;C:\Windows\SysWOW64\downlevel\
start VRServer.exe -w -m l1 -geometry 5120x1600+0+0 -display \\.\DISPLAY2
start VRServer.exe -w -m l2 -geometry 5120x1600+7680+0 -display \\.\DISPLAY6
start VRServer.exe -w -m l3 -geometry 5120x1600+5120+1600 -display \\.\DISPLAY10
start VRServer.exe -w -m l4 -geometry 5120x1600+0+1600 -display \\.\DISPLAY14
exit
