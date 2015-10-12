#!/bin/sh

#export LD_PRELOAD=/home/victor/Projects/polyvr/src/debugging/exception_preload.so
libs=/usr/lib/opensg:/usr/lib/CEF:/usr/lib/1.4:/usr/lib/virtuose

export LD_LIBRARY_PATH=$libs && ./bin/Debug/VRFramework $@
#export LD_LIBRARY_PATH=$libs && gdb bin/Debug/VRFramework
