#ifndef OSGGEOMETRY_H_INCLUDED
#define OSGGEOMETRY_H_INCLUDED

#include <OpenSG/OSGGeometry.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class OSGGeometry {
    public:
        GeometryMTRecPtr geo;

        OSGGeometry(GeometryMTRecPtr geo = 0);
        static OSGGeometryPtr create(GeometryMTRecPtr geo = 0);
};

OSG_END_NAMESPACE;

#endif // OSGGEOMETRY_H_INCLUDED
