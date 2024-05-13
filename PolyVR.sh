#!/bin/bash

osName=$(uname -s)

if [ "$osName" == "Darwin" ]; then # on mac
	libs=/usr/local/lib64
	export DYLD_LIBRARY_PATH="$libs$DYLD_LIBRARY_PATH"
else
	libs=/usr/lib/opensg:/usr/lib/CEF:/usr/lib/1.4:/usr/lib/virtuose:/usr/lib/STEPcode:/usr/lib/OCE:/usr/lib/OPCUA:/usr/lib/DWG
	export LD_LIBRARY_PATH="$libs$LD_LIBRARY_PATH"
fi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

if [ -e ./build/polyvr ]; then
	echo "run build/polyvr"
	 ./build/polyvr
fi

if [ -e ./bin/Debug/VRFramework ]; then
	./bin/Debug/VRFramework $@
else
	./bin/Release/VRFramework $@
fi
