#!/bin/bash

# creates a package for macOS
#  create an appbundle structure
#  package as dmg (diskimage)

#./pack_app3.sh Lernfabrik futurefactory.pvr /Users/victorhafner/Projects/lernfabrik

function addDir {
	if [ ! -e $1 ]; then
		mkdir -p $1
	fi
}

appName=$1
appProject=$2
appFolder=$3

if [ ! -d "$appFolder" ]; then
	echo "Error: app folder not found, $appFolder"
	exit 1
fi

if [ ! -e "$appFolder/$appProject" ]; then
	echo "Error: project file not found, $appFolder/$appProject"
	exit 1
fi

echo "get deploy config"
deployName="$appName"
deployExeName="$appName"

if [ -e "$appFolder/deploy/config" ]; then
	while IFS=: read -r key value; do
		echo $key $value
	  eval "${key}='${value}'"
	done < "$appFolder/deploy/config"
fi
echo "$deployName" "$deployExeName"

if [ -e "packages/$deployExeName.dmg" ]; then
	rm "packages/$deployExeName.dmg"
fi
if [ -e "packages/${deployExeName}_ro.dmg" ]; then
	rm "packages/${deployExeName}_ro.dmg"
fi

pckFolder="packages/$deployExeName.app"

addDir $pckFolder
rm -rf $pckFolder/*
addDir $pckFolder/Contents
addDir $pckFolder/Contents/MacOS
addDir $pckFolder/Contents/Resources
addDir $pckFolder/Contents/Frameworks

pckPVRFolder="$pckFolder/Contents/Resources" # copy directly in Resources because of CEF.. (relative path to helpers!)
bin="$pckFolder/Contents/MacOS/"
res="$pckFolder/Contents/Resources/"
libs="$pckFolder/Contents/Frameworks"

if true; then
	echo " copy app data"
	cp -r $appFolder/* $pckPVRFolder/
fi

if true; then
	echo " copy polyvr"
	cp -r build/polyvr $bin/
	cp -r ressources $res/
	cp -r setup $res/
	cp -r shader $res/
	#cp -r examples $res/

	mkdir -p "$libs/polyvr Helper.app/Contents/MacOS"
	mkdir -p "$libs/polyvr Helper (GPU).app/Contents/MacOS"
	mkdir -p "$libs/polyvr Helper (Renderer).app/Contents/MacOS"
	mkdir -p "$libs/polyvr Helper (Plugin).app/Contents/MacOS"
	mkdir -p "$libs/polyvr Helper (Alerts).app/Contents/MacOS"
	cp "ressources/cefMac/helper/CefSubProcessMac" "$libs/polyvr Helper.app/Contents/MacOS/polyvr Helper"
	cp "ressources/cefMac/helper/CefSubProcessMac (Alerts)" "$libs/polyvr Helper (Alerts).app/Contents/MacOS/polyvr Helper (Alerts)"
	cp "ressources/cefMac/helper/CefSubProcessMac (GPU)" "$libs/polyvr Helper (GPU).app/Contents/MacOS/polyvr Helper (GPU)"
	cp "ressources/cefMac/helper/CefSubProcessMac (Plugin)" "$libs/polyvr Helper (Plugin).app/Contents/MacOS/polyvr Helper (Plugin)"
	cp "ressources/cefMac/helper/CefSubProcessMac (Renderer)" "$libs/polyvr Helper (Renderer).app/Contents/MacOS/polyvr Helper (Renderer)"


	echo " copy libs"
	#cp -r $pyPath $pckFolder/engine/pyLibs
	cp -r /usr/local/lib64/* $libs/
	cp -r /usr/local/lib/cef/* $libs/
	cp -r /usr/local/lib/libcollada* $libs/

	echo " cleanup"
	rm -rf $bin/ressources/cef
	rm -rf $bin/ressources/cef18
	rm -rf $bin/ressources/cefWin

	if [ -e $pckPVRFolder/deploy/cleanup.sh ]; then
		/bin/bash $pckPVRFolder/deploy/cleanup.sh
	fi
fi

#exit 0

echo "create icon"
icon="ressources/gui/logo_icon.png"
if [ -e "$pckPVRFolder/deploy/icon.png" ]; then
	icon="$pckPVRFolder/deploy/icon.png"
fi

addDir $res/$deployExeName.iconset
cp $icon $res/$deployExeName.iconset/icon_16x16.png
cp $icon $res/$deployExeName.iconset/icon_16x16@2x.png
cp $icon $res/$deployExeName.iconset/icon_32x32.png
cp $icon $res/$deployExeName.iconset/icon_32x32@2x.png
cp $icon $res/$deployExeName.iconset/icon_128x128.png
cp $icon $res/$deployExeName.iconset/icon_128x128@2x.png
cp $icon $res/$deployExeName.iconset/icon_256x256.png
cp $icon $res/$deployExeName.iconset/icon_256x256@2x.png
cp $icon $res/$deployExeName.iconset/icon_512x512.png
cp $icon $res/$deployExeName.iconset/icon_512x512@2x.png
iconutil -c icns -o $res/$deployExeName.icns $res/$deployExeName.iconset

echo "write Info.plist file"
# check plist file with
#  plutil -lint Lernfabrik.app/Contents/Info.plist
cat <<EOT >> $pckFolder/Contents/Info.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleGetInfoString</key>
	<string>$deployExeName</string>
	<key>CFBundleExecutable</key>
	<string>startApp.sh</string>
	<key>CFBundleIdentifier</key>
	<string>com.ees.www</string>
	<key>CFBundleName</key>
	<string>$deployName</string>
	<key>CFBundleIconFile</key>
	<string>$deployExeName</string>
	<key>CFBundleShortVersionString</key>
	<string>0.01</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>IFMajorVersion</key>
	<integer>0</integer>
	<key>IFMinorVersion</key>
	<integer>1</integer>
	<key>LSMinimumSystemVersion</key>
	<string>10.12</string>
</dict>
</plist>
EOT

echo "write startApp.sh file"
if [ -n "$appProject" ]; then # check is appProject given
cat <<EOT >> $bin/startApp.sh
#!/bin/bash
DIR="\$(cd "\$(dirname "\$0")" && pwd)"
osascript <<EOF
tell application "Terminal"
    do script "cd \${DIR} && ./startApp2.sh"
end tell
EOF
EOT
fi

echo "write startApp2.sh file"
if [ -n "$appProject" ]; then # check is appProject given
cat <<EOT >> $bin/startApp2.sh
#!/bin/bash
#xclock
DIR="\$(cd "\$(dirname "\$0")" && pwd)"
LIBS="\${DIR}/../Frameworks"
libs="\$LIBS:\$LIBS/Chromium Embedded Framework.framework/Libraries"
export DYLD_LIBRARY_PATH="\$libs"
export DYLD_FRAMEWORK_PATH="\$libs"
cd \$DIR/../Resources
../MacOS/polyvr --setup="macOS" --application $appProject
EOT
fi

chmod +x $bin/startApp.sh
chmod +x $bin/startApp2.sh

echo "execute code signing"
codesign --force --deep --sign - $pckFolder


echo "create disk image"
hdiutil create -volname $deployExeName -srcfolder $pckFolder -ov -format UDRW "packages/$deployExeName.dmg"

echo "configure dmg"
MOUNT_POINT=/Volumes/$deployExeName
hdiutil detach $MOUNT_POINT
hdiutil attach "packages/$deployExeName.dmg"
ln -s /Applications $MOUNT_POINT/Applications

osascript <<EOF
tell application "Finder"
    tell disk "$deployExeName"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set the bounds of container window to {100, 100, 600, 400}
        set viewOptions to the icon view options of container window
        set arrangement of viewOptions to not arranged
        set icon size of viewOptions to 72
        set position of item "$deployExeName" of container window to {100, 100}
        set position of item "Applications" of container window to {400, 100}
        update without registering applications
        delay 5
    end tell
end tell
EOF

hdiutil detach $MOUNT_POINT
echo "create read only disk image"
hdiutil convert "packages/$deployExeName.dmg" -format UDZO -o "packages/${deployExeName}_ro.dmg"


echo " done"
