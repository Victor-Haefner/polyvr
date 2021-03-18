@echo off
set DIR=%~dp0
cd %DIR%
PsExec64.exe -accepteula -i -d -s \\127.0.0.1 CMD /C "%DIR%start-win.bat %*"
