#!/bin/bash

P=`dirname $(readlink -f $0)`

cID=$(git rev-parse HEAD)
cTi=$(git log -1 --format="%at" | xargs -I{} date -d @{} +%d.%m.%Y_%H:%M:%S)

echo " detected commit ID: %cID from %cTi"

vLine1="#define PVR_COMMIT_ID \"$cID\""
vLine2="#define PVR_COMMIT_TIME \"$cTi\""

if [ -f $P/version.h ]; then
	line=$(head -n 1 $P/version.h)

	if [ "$line" = "$vLine1" ]; then
		echo " version header up to date, skip.."
		exit 0
	fi

	echo " clear version header"
	rm $P/version.h
fi

echo " write version header"
echo $vLine1 >> $P/version.h
echo $vLine2 >> $P/version.h
