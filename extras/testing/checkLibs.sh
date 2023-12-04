#!/bin/sh

required_libs=$(ldd VRFramework | awk '{print $3}' | grep -v '^$')

for libPath in $required_libs; do
    lib=$(basename ${libPath})

    if [ ! $lib = "not" ]; then
    if [ ! -e "libs/$lib" ]; then
    	echo "cp $libPath \$engFolder/libs/"
    fi
    fi
done
