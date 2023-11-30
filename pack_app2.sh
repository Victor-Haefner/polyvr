#!/bin/bash

#./pack_app.sh PolyVR
#./pack_app.sh FinancialFolder conceptV1.pvr /c/Users/Victor/Projects/financialocean
#./pack_app.sh PoscarViewer poscarImport.pvr /c/Users/Victor/Projects/surfacechemistry

appName=$1
appProject=$2
appFolder=$3

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pckFolder="packages/"$appName

if [ ! -e $pckFolder ]; then
	mkdir -p $pckFolder 
fi

rm -rf $pckFolder/*
	
if [ -n "$appFolder" ]; then # check is appFolder given
	echo " copy app data"
	cp -r $appFolder/* $pckFolder/
fi

engFolder=$pckFolder"/engine"
engFolder=$pckFolder

if [ ! -e $engFolder ]; then
	mkdir -p $engFolder 
fi

echo " copy polyvr"
mkdir $engFolder/src
cp -r bin/Debug/VRFramework $engFolder/
cp -r src/cluster $engFolder/src/cluster
cp -r ressources $engFolder/ressources
cp -r setup $engFolder/setup
cp -r shader $engFolder/shader
cp -r examples $engFolder/examples
mkdir -p $engFolder/bin/Debug
cp bin/Debug/*.so $engFolder/bin/Debug/

echo " copy libs"
mkdir -p $engFolder/libs
cp -r /usr/lib/opensg/* $engFolder/libs/
cp -r /usr/lib/CEF/* $engFolder/libs/
cp -r /usr/lib/1.4/* $engFolder/libs/
cp -r /usr/lib/virtuose/* $engFolder/libs/
cp -r /usr/lib/STEPcode/* $engFolder/libs/
cp -r /usr/lib/OCE/* $engFolder/libs/
cp -r /usr/lib/OPCUA/* $engFolder/libs/
cp -r /usr/lib/DWG/* $engFolder/libs/

cat <<EOT >> $pckFolder/AppRun
#!/bin/sh
HERE="\$(dirname "\$(readlink -f "\${0}")")"
echo "AppRun PolyVR"
echo "work dir: '\$HERE'"
cd \$HERE
ls
export LD_LIBRARY_PATH="\${HERE}/libs:\${LD_LIBRARY_PATH}"
exec ./VRFramework "\$@"
EOT

chmod +x $pckFolder/AppRun

cat <<EOT >> $pckFolder/appimage.yml
app:
  name: PolyVR
  version: 1.0
  exec: PolyVR.sh
  icon: logo_icon.png
EOT

cat <<EOT >> $pckFolder/PolyVR.desktop
[Desktop Entry]
Type=Application
Name=PolyVR
Terminal=true
MimeType=application/x-polyvr
Categories=Development
Path=$DIR/$engFolder
Icon=logo_icon
EOT
#Icon=$DIR/$engFolder/ressources/gui/logo_icon

cp $engFolder/ressources/gui/logo_icon.png $pckFolder/logo_icon.png


echo " done"

