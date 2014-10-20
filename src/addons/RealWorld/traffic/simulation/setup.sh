g++ -shared -fPIC -I/usr/include/python2.7 Traffic.c -lTrafficSim -Wl,--as-needed -o Traffic.so
