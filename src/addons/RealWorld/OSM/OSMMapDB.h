#ifndef OSMMAPDB_H
#define OSMMAPDB_H

#include "OSMMap.h"

using namespace std;

struct OSMMapDB {
    map<string, OSMMap*> maps;

    OSMMap* getMap(string posStr);
};

#endif // OSMMAPDB_H

