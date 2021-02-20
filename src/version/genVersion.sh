#!/bin/bash

cID=$(git rev-parse HEAD)
cTi=$(git log -1 --format="%at" | xargs -I{} date -d @{} +%d.%m.%Y_%H:%M:%S)
echo $cID, $cTi

if [ -f version.h ]; then
    rm version.h
fi

P=`dirname $(readlink -f $0)`

echo "#define PVR_COMMIT_ID \"$cID\"" >> $P/version.h
echo "#define PVR_COMMIT_TIME \"$cTi\"" >> $P/version.h
