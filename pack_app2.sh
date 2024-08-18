#!/bin/bash

#./pack_app2.sh polyvr && ./appimagetool-x86_64.AppImage -n packages/polyvr
#./pack_app2.sh PoscarViewer poscarImport.pvr /c/Users/Victor/Projects/surfacechemistry

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
cp -r bin/Debug/*.bin $engFolder/
cp -r bin/Debug/*.dat $engFolder/
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
cp -r /usr/lib/x86_64-linux-gnu/nss/* $engFolder/libs/

echo " copy system libs"
syslibs=$(ldd bin/Debug/VRFramework | awk 'NF == 4 {print $3}; NF == 2 {print $1}');

syslibs_paths=()
while IFS= read -r line; do
    syslibs_paths+=("$line")
done <<< "$syslibs"

syslibs_cp_paths=()
for i in "${syslibs_paths[@]}"; do
	if [[ "$i" == "not"* ]]; then continue; fi
	if [[ "$i" == *"libstdc++.so"* ]]; then continue; fi
	if [[ "$i" == *"libgcc_s.so"* ]]; then continue; fi
	if [[ "$i" == *"libc.so"* ]]; then continue; fi
	if [[ "$i" == *"libz.so"* ]]; then continue; fi
	if [[ "$i" == *"libm.so"* ]]; then continue; fi
	if [[ "$i" == *"libX11.so"* ]]; then continue; fi
	if [[ "$i" == *"libGL.so"* ]]; then continue; fi
	if [[ "$i" == *"libxcb.so"* ]]; then continue; fi
	if [[ "$i" == *"libgmp.so"* ]]; then continue; fi
	if [[ "$i" == *"libGLU.so"* ]]; then continue; fi
	if [[ "$i" == *"libglib-2.0.so"* ]]; then continue; fi
	if [[ "$i" == *"libGLdispatch.so"* ]]; then continue; fi
	if [[ "$i" == *"libGLX.so"* ]]; then continue; fi
	if [[ "$i" == *"libOpenGL.so"* ]]; then continue; fi
	if [[ "$i" == *"ld-linux-x86-64.so"* ]]; then continue; fi
	if [[ "$i" == "/usr/lib/OCE"* ]]; then continue; fi
	
	if [[ "$i" == "/lib"* ]]; then syslibs_cp_paths+=("$i"); fi
	if [[ "$i" == "/usr"* ]]; then syslibs_cp_paths+=("$i"); fi
done

for path in "${syslibs_cp_paths[@]}"; do
    cp "$path" $engFolder/libs/
done
cp /usr/lib/x86_64-linux-gnu/libglut.so.3 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/nss/* $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libboost_thread* $engFolder/libs/


rm -rf $engFolder/libs/CMakeFiles


if [ -e $pckFolder/cleanupDeploy.sh ]; then
	/bin/bash $pckFolder/cleanupDeploy.sh 
fi



if [ -n "$appProject" ]; then # check is appProject given
cat <<EOT >> $pckFolder/AppRun
#!/bin/sh
HERE="\$(dirname "\$(readlink -f "\${0}")")"
echo "AppRun PolyVR"
echo "work dir: '\$HERE'"
cd \$HERE
ls
export LD_LIBRARY_PATH="\${HERE}/libs:\${LD_LIBRARY_PATH}"
exec ./VRFramework --dofailcheck=0 --maximized=1 --application $appProject "\$@"
EOT
else
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
fi



chmod +x $pckFolder/AppRun

cat <<EOT >> $pckFolder/appimage.yml
app:
  name: $appName
  version: 1.0
  exec: PolyVR.sh
  icon: logo_icon.png
EOT

cat <<EOT >> $pckFolder/PolyVR.desktop
[Desktop Entry]
Type=Application
Name=$appName
Terminal=true
MimeType=application/x-polyvr
Categories=Development
Path=$DIR/$engFolder
Icon=logo_icon
EOT
#Icon=$DIR/$engFolder/ressources/gui/logo_icon

cp $engFolder/ressources/gui/logo_icon.png $pckFolder/logo_icon.png


echo " done"

