#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$DIR
#export PATH=$PATH:$DIR
g++ -shared -fPIC -I/usr/include/python2.7 Traffic.c -L$DIR -lTrafficSim -Wl,--as-needed -o Traffic.so
