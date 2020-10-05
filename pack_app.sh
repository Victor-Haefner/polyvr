#!/bin/bash

# TODO: pass those as parameters!
appName="FinancialFolder"
appFolder="/c/Users/Victor/Projects/financialocean"
appProject="conceptV1.pvr"

pckFolder="packages/"$appName

if [ ! -e $pckFolder ]; then
	mkdir -p $pckFolder 
fi

rm -rf $pckFolder/*

echo " copy app data"
cp -r $appFolder/* $pckFolder/

echo " copy polyvr"
mkdir $pckFolder/engine
cp -r build/Release/* $pckFolder/engine/
cp -r ressources $pckFolder/engine/ressources
cp -r setup $pckFolder/engine/setup
cp -r shader $pckFolder/engine/shader


cat <<EOT >> $pckFolder/startApp.bat
@echo off

set PATH=%PATH%;C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x64;C:\usr\lib\opensg;C:\usr\vcpkg\installed\x64-windows\lib;C:\usr\lib\cef;
set PYTHONPATH=%PYTHONPATH%;C:\usr\vcpkg\installed\x64-windows\share\python2\Lib
cd engine
polyvr.exe --application ../$appProject
EOT


echo " done"