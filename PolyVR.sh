#!/bin/sh

libs=/usr/lib/opensg:/usr/lib/CEF:/usr/lib/1.4:/usr/lib/virtuose

export LD_LIBRARY_PATH=$libs && ./bin/Debug/VRFramework $@
