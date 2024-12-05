#!/bin/bash

# creates a package for macOS
#  create an appbundle structure
#  package as dmg (diskimage)

#./pack_app3.sh Lernfabrik futurefactory.pvr /Users/victorhafner/Projects/lernfabrik

appName=$1
appProject=$2
appFolder=$3
deployName="$appName"
deployExeName="$appName"
pckFolder="packages/$deployExeName.app"
pckPVRFolder="$pckFolder/Contents/Resources" # copy directly in Resources because of CEF.. (relative path to helpers!)
bin="$pckFolder/Contents/MacOS/"
res="$pckFolder/Contents/Resources/"
libs="$pckFolder/Contents/Frameworks"

function signFile {
	# Warning, using --options runtime makes apple apply strikt security rules, this may make the app not to start at all
	codesign --force --options runtime --sign "Developer ID Application: Victor Haefner" --deep "$1"
	#codesign --force --sign - --deep "$1" # for testing
}

function signBundle {
	echo "sign whole bundle, $pckFolder"
	codesign --force --options runtime --sign "Developer ID Application: Victor Haefner" --deep "$pckFolder"
	#codesign --force --sign - --deep "$pckFolder" # for testing, see above
}

strip_absolute_paths() {
    local executable=$1
    #local libs=$(otool -L "$executable" | grep -oE '/[^ ]+\.dylib' | sort -u)
    local libs=$(otool -L "$executable" | grep -oE '[^ ]+\.dylib' | sort -u)

    # Loop through each library and strip the absolute path
    for lib in $libs; do
        # TODO: check if boost or boost@1.76 in path and if present prepend to lib_name!

				local lib_name=$(basename "$lib")

				if [[ "$lib" == *"libSystem"* ]]; then
						continue
				fi

				if [[ "$lib" == *"libobjc"* ]]; then
						continue
				fi

				if [[ "$lib" == *"libc++"* ]]; then
						continue
				fi

				#if [[ "$lib" == *"libgfortran"* ]]; then
				#		continue
				#fi

				if [[ ! -f $2/$lib_name ]]; then
					  echo "strip $executable, ignore lib $lib, $lib_name is not in folder $2!"
						continue # ignore libs not in folder
				fi

				if [[ "$lib" == *"boost@1.76"* ]]; then
            lib_name="boost@1.76/$lib_name"
						#continue
				elif [[ "$lib" == *"boost"* ]]; then
            lib_name="boost/$lib_name"
						#continue
        fi

				rPath="@executable_path/../Frameworks"

        install_name_tool -change "$lib" "$rPath/$lib_name" "$executable" 2>/dev/null
    done
}

function check_libs_paths {
		echo "check_libs_paths"
		for file in $1/*.dylib; do
        if [[ -f $file ]]; then
            #echo "Processing file: $file"
            strip_absolute_paths "$file" "$1"
						signFile $file
        fi
    done

		for file in $1/boost176/*.dylib; do
				if [[ -f $file ]]; then
						#echo "Processing file: $file"
						#strip_absolute_paths "$file" "$1"
						signFile $file
				fi
		done
}

function addDir {
	if [ ! -e $1 ]; then
		mkdir -p $1
	fi
}

function checkAppFolder {
	echo "check for $appFolder/$appProject"

	if [ ! -d "$appFolder" ]; then
		echo "Error: app folder not found, $appFolder"
		exit 1
	fi

	if [ ! -e "$appFolder/$appProject" ]; then
		echo "Error: project file not found, $appFolder/$appProject"
		exit 1
	fi
}

function getDeployConfig {
	echo "get deploy config"

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
}

function setupFolders {
	addDir $pckFolder
	rm -rf $pckFolder/*
	addDir $pckFolder/Contents
	addDir $pckFolder/Contents/MacOS
	addDir $pckFolder/Contents/Resources
	addDir $pckFolder/Contents/Frameworks
}

function copyAppData {
	echo " copy app data"
	cp -r $appFolder/* $pckPVRFolder/
}

function copyPolyVR {
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
}

function signPolyVR {
	signFile $bin/polyvr

	signFile "$res/ressources/cefMac/helper/CefSubProcessMac"
	signFile "$res/ressources/cefMac/helper/CefSubProcessMac (Alerts)"
	signFile "$res/ressources/cefMac/helper/CefSubProcessMac (GPU)"
	signFile "$res/ressources/cefMac/helper/CefSubProcessMac (Plugin)"
	signFile "$res/ressources/cefMac/helper/CefSubProcessMac (Renderer)"

	signFile "$libs/polyvr Helper.app/Contents/MacOS/polyvr Helper"
	signFile "$libs/polyvr Helper (Alerts).app/Contents/MacOS/polyvr Helper (Alerts)"
	signFile "$libs/polyvr Helper (GPU).app/Contents/MacOS/polyvr Helper (GPU)"
	signFile "$libs/polyvr Helper (Plugin).app/Contents/MacOS/polyvr Helper (Plugin)"
	signFile "$libs/polyvr Helper (Renderer).app/Contents/MacOS/polyvr Helper (Renderer)"

	signFile "$libs/Chromium Embedded Framework.framework/Libraries/libEGL.dylib"
	signFile "$libs/Chromium Embedded Framework.framework/Libraries/libvk_swiftshader.dylib"
	signFile "$libs/Chromium Embedded Framework.framework/Libraries/libGLESv2.dylib"
}

function copyDependencies {
		echo " copy libs"
		#cp -r $pyPath $pckFolder/engine/pyLibs
		cp -r /usr/local/lib64/* $libs/
		rm $libs/libOSGWindowGLUT*
		cp -r /usr/local/lib/cef/* $libs/
		cp -r /usr/local/lib/libcollada* $libs/
		cp -r /opt/homebrew/opt/freetype/lib/* $libs
		cp -r $HOME/.pyenv/versions/2.7.18/lib/* $libs/
		cp -r /opt/homebrew/opt/bullet/lib/* $libs/
		cp -r /opt/homebrew/opt/icu4c/lib/* $libs/
		cp -r /opt/homebrew/opt/lapack/lib/* $libs/
		cp -r /opt/homebrew/opt/openal-soft/lib/* $libs/
		cp -r /opt/homebrew/opt/ffmpeg/lib/* $libs/
		cp -r /opt/homebrew/opt/fftw/lib/* $libs/
		cp -r /opt/homebrew/opt/jsoncpp/lib/* $libs/
		cp -r /opt/homebrew/opt/libssh/lib/* $libs/
		cp -r /opt/homebrew/opt/libpng/lib/* $libs/
		cp -r /opt/homebrew/opt/jpeg-turbo/lib/* $libs/
		cp -r /opt/homebrew/opt/krb5/lib/* $libs/

	  # TODO: those should be present on mac by default, but maybe the version might not fit someday?
		#cp /usr/lib/libcurl.4.dylib $libs/
		#cp /usr/lib/libz.1.dylib $libs/
		#cp /usr/lib/libbz2.1.0.dylib $libs/
		#cp /usr/lib/libiconv.2.dylib $libs/

		while IFS= read -r line; do
	    cp -r "$line" $libs/
		done < "macLibs.txt"

		rm -f $libs/*.a

		mkdir $libs/lib
		mv $libs/python2.7 $libs/lib/python27
		ln -s python27 python2.7 ; mv ./python2.7 $libs/lib/python2.7

		mv $libs/python3.11 $libs/python311
		mv $libs/bullet/single/python3.11 $libs/bullet/single/python311
		ln -s python311 python3.11 ; mv ./python3.11 $libs/python3.11
		ln -s python311 python3.11 ; mv ./python3.11 $libs/bullet/single/python3.11

		mkdir -p "$libs/boost"
		mkdir -p "$libs/boost@1.76"
	  cp -r /opt/homebrew/opt/boost/lib/* $libs/boost/
		cp -r /opt/homebrew/opt/boost@1.76/lib/* $libs/boost@1.76/
		cp $libs/boost/libboost_system.dylib $libs/
		cp $libs/boost/libboost_serialization.dylib $libs/
		cp $libs/boost/libboost_filesystem.dylib $libs/
		cp $libs/boost/libboost_program_options.dylib $libs/

		mv $libs/boost@1.76 $libs/boost176
		ln -s boost176 boost@1.76 ; mv ./boost@1.76 $libs/boost@1.76

		echo " cleanup"
		rm -rf $res/ressources/cef
		rm -rf $res/ressources/cef18
		rm -rf $res/ressources/cefWin
		rm -rf $libs/pkgconfig
		rm -rf $libs/cmake
		rm -f $libs/boost/*.a
		rm -rf $libs/boost/cmake
		rm -rf $libs/bullet/single/pkgconfig
		rm -rf $libs/bullet/single/cmake
		rm -f $libs/bullet/single/*.a
		rm -rf $libs/bullet/double/pkgconfig
		rm -rf $libs/bullet/double/cmake
		rm -f $libs/bullet/double/*.a
		rm -rf $libs/lib/python27/site-packages/*.dist-info
		rm -f $libs/lib/python27/config/libpython2.7.a
		rm -f $libs/boost176/*.a
		rm -rf $libs/boost176/cmake
		rm -rf $libs/icu

		if [ -e $pckPVRFolder/deploy/cleanup.sh ]; then
			/bin/bash $pckPVRFolder/deploy/cleanup.sh
		fi
}

function stripPaths {
		check_libs_paths $libs
		strip_absolute_paths $bin/polyvr "$libs"
}

function setupAppRessources {
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
#!/bin/zsh
DIR="\$(cd "\$(dirname "\$0")" && pwd)"
osascript <<EOF
tell application "Terminal"
    do script "cd \${DIR} && ./startApp2.sh"
end tell
EOF
#cd \${DIR} && ./startApp2.sh
EOT
	fi

	echo "write startApp2.sh file"
	if [ -n "$appProject" ]; then # check is appProject given
	cat <<EOT >> $bin/startApp2.sh
#!/bin/zsh
#xclock
DIR="\$(cd "\$(dirname "\$0")" && pwd)"
LIBS="\${DIR}/../Frameworks"
libs="\$LIBS:\$LIBS/Chromium Embedded Framework.framework/Libraries"
export DYLD_LIBRARY_PATH="\$libs"
export DYLD_FRAMEWORK_PATH="\$libs"
export PYTHONHOME="\$LIBS"
cd \$DIR/../Resources
../MacOS/polyvr --setup="macOS" --application $appProject
EOT
	fi

	chmod +x $bin/startApp.sh
	chmod +x $bin/startApp2.sh
}

function verifyApp {
	echo "zip app"
	/usr/bin/ditto -c -k --keepParent "packages/$deployExeName.app" "packages/$deployExeName.zip"

	echo "submit packages/$deployExeName.zip for verification"
	xcrun notarytool submit "packages/$deployExeName.zip" --keychain-profile "notarizeLernfabrik" --wait
	xcrun stapler staple "packages/$deployExeName.app"
	xcrun notarytool history --keychain-profile "notarizeLernfabrik"
}

function createDiskImage {
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
}

checkAppFolder
getDeployConfig
setupFolders
setupAppRessources
copyAppData
copyPolyVR
copyDependencies
stripPaths
signPolyVR
signBundle
verifyApp
createDiskImage

echo " done"
