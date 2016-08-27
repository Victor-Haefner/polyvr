#include "OSMMapDB.h"
#include <iostream>
#include <boost/filesystem.hpp>

OSMMap* OSMMapDB::getMap(string posStr) {
    if (maps.count(posStr)) return maps[posStr];

    string filename = "world/mapdata/map-"+posStr+".osm";
    if ( !boost::filesystem::exists(filename) ) { cout << "OSMMapDB Error: no file " << filename << endl; return 0; }

    maps[posStr] = OSMMap::loadMap(filename);
    return maps[posStr];
}
