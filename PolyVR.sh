#!/bin/bash

osName=$(uname -s)

if [ "$osName" == "Darwin" ]; then # on mac
	libs=/usr/local/lib64:/usr/local/lib/cef
	export DYLD_LIBRARY_PATH="$libs$DYLD_LIBRARY_PATH"
	export DYLD_FRAMEWORK_PATH="$libs$DYLD_FRAMEWORK_PATH" #  needed to find the cef framework
else
	libs=/usr/lib/opensg:/usr/lib/CEF:/usr/lib/1.4:/usr/lib/virtuose:/usr/lib/STEPcode:/usr/lib/OCE:/usr/lib/OPCUA:/usr/lib/DWG
	export LD_LIBRARY_PATH="$libs$LD_LIBRARY_PATH"
fi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

if [ -e ./build/polyvr ]; then
	 echo "run build/polyvr"
	 #otool -L ./build/polyvr
	 ./build/polyvr $@
	 # Create a temporary lldb init file
	 #TEMP_LDBINIT=$(mktemp)
	 #echo "settings set target.env-vars DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH DYLD_FRAMEWORK_PATH=$DYLD_FRAMEWORK_PATH" > $TEMP_LDBINIT
	 #lldb --source $TEMP_LDBINIT -o run -- ./build/polyvr $@ # mac debugger
	 exit 0
fi

if [ -e ./bin/Debug/VRFramework ]; then
	./bin/Debug/VRFramework $@
	exit 0
else
	./bin/Release/VRFramework $@
	exit 0
fi
