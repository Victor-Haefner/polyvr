#ifndef OSMMAPDB_H
#define OSMMAPDB_H

#include "OSMMap.h"

using namespace std;

class OSMMapDB {
    public:
        map<string, OSMMap*> maps;

        OSMMap* getMap(string posStr);
};

#endif // OSMMAPDB_H

