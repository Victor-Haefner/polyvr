#include "OSMMapDB.h"
#include "../RealWorld.h"
#include <iostream>
#include <boost/filesystem.hpp>

OSMMap* OSMMapDB::getMap(string posStr) {
    if (maps.count(posStr)) return maps[posStr];

    string chunkspath = RealWorld::getOption("CHUNKS_PATH");
    if (*chunkspath.rbegin() != '/') chunkspath += "/";
    string filename = chunkspath+"map-"+posStr+".osm";
    if ( !boost::filesystem::exists(filename) ) { cout << "OSMMapDB Error: no file " << filename << endl; return 0; }

    maps[posStr] = OSMMap::loadMap(filename);
    return maps[posStr];
}
