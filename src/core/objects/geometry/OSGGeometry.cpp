#include "OSGGeometry.h"

using namespace OSG;

OSGGeometry::OSGGeometry(GeometryMTRecPtr geo) {
    this->geo = geo;
}

OSGGeometryPtr OSGGeometry::create(GeometryMTRecPtr geo) { return OSGGeometryPtr( new OSGGeometry(geo) ); }
