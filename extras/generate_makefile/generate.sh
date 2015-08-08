#!/bin/bash

if (test -e ../../PolyVR.cbp) && (test -e ./cbp2make.linux-x86_64) then
	./cbp2make.linux-x86_64 -in ../../PolyVR.cbp -out ../../Makefile
elif (test -e ./PolyVR.cbp) then
	./extras/generate_makefile/cbp2make.linux-x86_64 -in PolyVR.cbp -out Makefile
elif (test -e ../PolyVR.cbp) && (test $(basename $(pwd)) = "extras") then
	./generate_makefile/cbp2make.linux-x86_64 -in ../PolyVR.cbp -out ../Makefile
else 
	echo "This did not work. Run from repository root or extras/ or extras/generate_makefile/"
fi

