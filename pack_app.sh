#!/bin/bash

#./pack_app.sh PolyVR
#./pack_app.sh FinancialFolder conceptV1.pvr /c/Users/Victor/Projects/financialocean
#./pack_app.sh PoscarViewer poscarImport.pvr /c/Users/Victor/Projects/surfacechemistry

appName=$1
appProject=$2
appFolder=$3

pckFolder="packages/"$appName

pyPath="/c/usr/vcpkg/installed/x64-windows/share/python2/Lib"
redistPath="/c/Program Files (x86)/Windows Kits/10/Redist/10.0.19041.0/ucrt/DLLs/x64"
vcpkgLibs="/c/usr/vcpkg/installed/x64-windows/lib"

if [ ! -e $pckFolder ]; then
	mkdir -p $pckFolder 
fi

rm -rf $pckFolder/*
	
if [ -n "$appFolder" ]; then # check is appFolder given
	echo " copy app data"
	cp -r $appFolder/* $pckFolder/
else # sign polyvr executable
	if [ -e sign_polyvr.sh ]; then
		./sign_polyvr.sh
	fi
fi

echo " copy polyvr"
mkdir $pckFolder/engine
mkdir $pckFolder/engine/src
cp -r build/Release/* $pckFolder/engine/
cp -r src/cluster $pckFolder/engine/src/cluster
cp -r ressources $pckFolder/engine/ressources
cp -r setup $pckFolder/engine/setup
cp -r shader $pckFolder/engine/shader
cp -r examples $pckFolder/engine/examples

echo " copy libs"
cp -r $pyPath $pckFolder/engine/pyLibs
mkdir -p $pckFolder/engine/libs
cp -r "$redistPath"/* $pckFolder/engine/libs/
cp -r /c/Windows/system32/MSVCP140.dll $pckFolder/engine/libs/
cp -r /c/Windows/system32/VCRUNTIME140.dll $pckFolder/engine/libs/
cp -r /c/Windows/system32/VCRUNTIME140_1.dll $pckFolder/engine/libs/
cp -r /c/usr/lib/opensg/* $pckFolder/engine/libs/
cp -r /c/usr/lib/cef/* $pckFolder/engine/libs/
cp -r /c/usr/lib/oce/* $pckFolder/engine/libs/
cp -r /c/usr/lib/ifc/* $pckFolder/engine/libs/
cp -r /c/usr/lib/openvr/* $pckFolder/engine/libs/
cp -r /c/usr/lib/collada/* $pckFolder/engine/libs/
cp -r "$vcpkgLibs"/* $pckFolder/engine/libs/

echo " cleanup"
rm $pckFolder/engine/polyvr.pdb
rm $pckFolder/engine/libs/*.pdb
rm -rf $pckFolder/engine/ressources/cef
rm -rf $pckFolder/engine/ressources/cef18


if [ -n "$appProject" ]; then # check is appProject given
cat <<EOT >> $pckFolder/startApp.bat
@echo off
set PATH=%PATH%;%~f0\..\engine\libs;
set PYTHONPATH=%PYTHONPATH%;%~f0\..\engine\pyLibs
cd engine
polyvr.exe --maximized=1 --application ../$appProject
EOT
cat <<EOT >> $pckFolder/startAppNoTerm.vbs
Set WshShell = CreateObject("WScript.Shell")
WshShell.Environment("Process")("PATH") = WshShell.Environment("Process")("PATH") & ";" & WshShell.CurrentDirectory & "\engine\libs"
WshShell.Environment("Process")("PYTHONPATH") = WshShell.Environment("Process")("PYTHONPATH") & ";" & WshShell.CurrentDirectory & "\engine\pyLibs"
WshShell.CurrentDirectory = WshShell.CurrentDirectory & "\engine"
WshShell.Run "polyvr.exe --maximized=1 --application ../$appProject", 0
Set WshShell = Nothing
EOT
else
cat <<EOT >> $pckFolder/startApp.bat
@echo off
set PATH=%PATH%;%~f0\..\engine\libs;
set PYTHONPATH=%PYTHONPATH%;%~f0\..\engine\pyLibs
cd engine
polyvr.exe
EOT
fi

if [ -e $pckFolder/deploy/cleanup.sh ]; then
	/bin/bash $pckFolder/deploy/cleanup.sh
fi

#polyvr.exe --standalone=1 --fullscreen=1 --application ../$appProject
#polyvr.exe --standalone=1 --application ../$appProject

echo " done"

# package the folder $pckFolder with inno setup compiler