#!/bin/bash

#export LD_PRELOAD=/home/victor/Projects/polyvr/src/debugging/exception_preload.so
libs=/usr/lib/opensg:/usr/lib/CEF:/usr/lib/1.4:/usr/lib/virtuose:/usr/lib/STEPcode:/usr/lib/OCE:/usr/lib/OPCUA:/usr/lib/DWG

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

if [ -e ./bin/Debug/VRFramework ]; then
	export LD_LIBRARY_PATH=$libs && ./bin/Debug/VRFramework $@
else
	export LD_LIBRARY_PATH=$libs && ./bin/Release/VRFramework $@
fi
