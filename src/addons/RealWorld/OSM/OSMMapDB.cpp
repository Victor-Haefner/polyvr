#include "OSMMapDB.h"
#include <iostream>
#include <boost/filesystem.hpp>

OSMMap* OSMMapDB::getMap(string posStr) {
    if (maps.count(posStr)) return maps[posStr];

    string filename = "world/mapdata/map-"+posStr+".osm";

    if ( !boost::filesystem::exists(filename) ) {
        cout << "MAP FILE NOT FOUND: " << filename << "\n" << flush;
        return 0;
    }

    maps[posStr] = OSMMap::loadMap(filename);
    return maps[posStr];

    // TODO: Garbage Collection. While ->maps has more than 30 entries, remove least recently used.
}
