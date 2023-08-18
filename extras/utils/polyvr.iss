[Setup]
AppName=PolyVR
AppVersion=1.0
WizardStyle=modern
DefaultDirName={autopf}\PolyVR
DefaultGroupName=PolyVR
UninstallDisplayIcon={app}\engine\ressources\gui\logo_icon_win.ico
Compression=lzma2
SolidCompression=yes
OutputDir=..\..\packages
OutputBaseFilename=polyvr
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "..\..\packages\polyvr\*"; \
DestDir: "{app}\"; \
Flags: ignoreversion recursesubdirs createallsubdirs onlyifdoesntexist

[Icons]
Name: "{group}\PolyVR"; Filename: "{app}\startApp.bat"; WorkingDir: "{app}"; IconFilename: "{app}\engine\ressources\gui\logo_icon_win.ico"
