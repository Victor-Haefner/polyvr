#ifndef OSMMAPDB_H
#define OSMMAPDB_H

#include "OSMMap.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class OSMMapDB {
    public:
        map<string, OSMMap*> maps;

        OSMMap* getMap(string posStr);
};

OSG_END_NAMESPACE;

#endif // OSMMAPDB_H

