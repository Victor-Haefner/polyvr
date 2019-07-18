
set PATH=%PATH%;C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x64;C:\opensg;C:\freeglut;C:\boost\lib64-msvc-14.1;C:\zlib
start VRServer.exe -w -m l1 -geometry 1920x1200+0+0 -display \\.\DISPLAY2
start VRServer.exe -w -m l2 -geometry 1920x1200+1920+0 -display \\.\DISPLAY6
start VRServer.exe -w -m l3 -geometry 1920x1200+3840+0 -display \\.\DISPLAY10
start VRServer.exe -w -m l4 -geometry 1920x1200+5760+0 -display \\.\DISPLAY14
exit