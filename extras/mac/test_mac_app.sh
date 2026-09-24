#!/bin/bash

root="../mcCefTest.app"

if [ -d $root ]; then
	rm -rf $root
fi

bin="$root/Contents/MacOS"
res="$root/Contents/Resources"
frw="$root/Contents/Frameworks"

mkdir -p $bin
mkdir -p $res
mkdir -p $frw

cp build/polyvr $bin/
cp -r setup $bin/
cp -r shader $bin/
cp -r ressources $bin/
cp -r examples/CEF.xml $bin/

mkdir -p "$frw/polyvr Helper.app/Contents/MacOS"
mkdir -p "$frw/polyvr Helper (GPU).app/Contents/MacOS"
mkdir -p "$frw/polyvr Helper (Renderer).app/Contents/MacOS"
mkdir -p "$frw/polyvr Helper (Plugin).app/Contents/MacOS"
mkdir -p "$frw/polyvr Helper (Alerts).app/Contents/MacOS"
cp "ressources/cefMac/helper/CefSubProcessMac" "$frw/polyvr Helper.app/Contents/MacOS/polyvr Helper"
cp "ressources/cefMac/helper/CefSubProcessMac (Alerts)" "$frw/polyvr Helper (Alerts).app/Contents/MacOS/polyvr Helper (Alerts)"
cp "ressources/cefMac/helper/CefSubProcessMac (GPU)" "$frw/polyvr Helper (GPU).app/Contents/MacOS/polyvr Helper (GPU)"
cp "ressources/cefMac/helper/CefSubProcessMac (Plugin)" "$frw/polyvr Helper (Plugin).app/Contents/MacOS/polyvr Helper (Plugin)"
cp "ressources/cefMac/helper/CefSubProcessMac (Renderer)" "$frw/polyvr Helper (Renderer).app/Contents/MacOS/polyvr Helper (Renderer)"

libs="/usr/local/lib64:/usr/local/lib/cef:/usr/local/lib/cef//Chromium Embedded Framework.framework/Libraries"
export DYLD_LIBRARY_PATH="$libs$DYLD_LIBRARY_PATH"
export DYLD_FRAMEWORK_PATH="$libs$DYLD_FRAMEWORK_PATH" #  needed to find the cef framework
cd $bin && ./polyvr --setup="macOS" --application ./CEF.xml


#cefclient.app/
#  Contents/
#    Frameworks/
#      Chromium Embedded Framework.framework/
#        Chromium Embedded Framework <= main application library
#        Libraries/
#          libEGL.dylib <= ANGLE support libraries
#          libGLESv2.dylib <=^
#          libvk_swiftshader.dylib <= SwANGLE support libraries
#          vk_swiftshader_icd.json <=^
#        Resources/
#          chrome_100_percent.pak <= non-localized resources and strings
#          chrome_200_percent.pak <=^
#          resources.pak          <=^
#          gpu_shader_cache.bin <= ANGLE-Metal shader cache
#          icudtl.dat <= unicode support
#          snapshot_blob.bin, v8_context_snapshot.[x86_64|arm64].bin <= V8 initial snapshot
#          en.lproj/, ... <= locale-specific resources and strings
#          Info.plist
#      cefclient Helper.app/
#        Contents/
#          Info.plist
#          MacOS/
#            cefclient Helper <= helper executable
#          Pkginfo
#      Info.plist
#    MacOS/
#      cefclient <= cefclient application executable
#    Pkginfo
#    Resources/
#      binding.html, ... <= cefclient application resources
