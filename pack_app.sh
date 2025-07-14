#!/bin/bash

#./pack_app.sh PolyVR
#./pack_app.sh FinancialFolder conceptV1.pvr /c/Users/Victor/Projects/financialocean
#./pack_app.sh PoscarViewer poscarImport.pvr /c/Users/Victor/Projects/surfacechemistry
#./pack_app.sh Lernfabrik futurefactory.pvr /c/Users/victo/Projects/lernfabrik

appName=$1
appProject=$2
appFolder=$3

pckFolder="packages/"$appName

vcpkgLibs="/c/usr/vcpkg/installed/x64-windows/lib"
pyPath="/c/usr/vcpkg/installed/x64-windows/share/python2/Lib"
redistPath="/c/Program Files (x86)/Windows Kits/10/Redist/10.0.19041.0/ucrt/DLLs/x64"
signtoolPath="/c/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/signtool.exe"

check_signature() {
    "$signtoolPath" verify -pa "$1" &> /dev/null
    if [ $? -eq 0 ]; then
		echo "checked signature of $1, it is signed"
        return 0 # "The executable is signed."
    else
		echo "checked signature of $1, it is NOT signed"
        return 1 # "The executable is not signed."
    fi
}

sign_polyvr() {
	check_signature build/Release/polyvr.exe
	if [ $? -eq 1 ]; then
		if [ -e codeSigning/sign_polyvr.sh ]; then
			echo "sign polyvr!"
			cd codeSigning
			./sign_polyvr.sh
			cd ..
		fi
	fi
}

if [ ! -e $pckFolder ]; then
	mkdir -p $pckFolder
fi

rm -rf $pckFolder/*

sign_polyvr

if [ -n "$appFolder" ]; then # check is appFolder given
	echo " copy app data"
	cp -r $appFolder/* $pckFolder/
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
polyvr.exe --dofailcheck=0 --maximized=1 --application ../$appProject
EOT
cat <<EOT >> $pckFolder/startAppNoTerm.vbs
Set WshShell = CreateObject("WScript.Shell")
WshShell.Environment("Process")("PATH") = WshShell.Environment("Process")("PATH") & ";" & WshShell.CurrentDirectory & "\engine\libs"
WshShell.Environment("Process")("PYTHONPATH") = WshShell.Environment("Process")("PYTHONPATH") & ";" & WshShell.CurrentDirectory & "\engine\pyLibs"
WshShell.CurrentDirectory = WshShell.CurrentDirectory & "\engine"
WshShell.Run "polyvr.exe  --dofailcheck=0 --maximized=1 --application ../$appProject", 0
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
