#ifndef STREETLAMP_H_INCLUDED
#define STREETLAMP_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "Asset.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGeoData;
class Pose;

class StreetLamp : public Asset {
    public:
        static void make();
        static void add(const Pose& p, VRGeoData* geo);
};

OSG_END_NAMESPACE;

#endif // STREETLAMP_H_INCLUDED
