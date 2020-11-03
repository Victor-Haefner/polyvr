#ifndef ASSET_H_INCLUDED
#define ASSET_H_INCLUDED

#include <map>
#include <string>
#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGeoData;
class pose;

class Asset {
    public:
        static map<string, VRGeometryPtr> assets;
        static VRGeometryPtr merge(VRObjectPtr obj);

    public:
        ;
};

OSG_END_NAMESPACE;

#endif // ASSET_H_INCLUDED
