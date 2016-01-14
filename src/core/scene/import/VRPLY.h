#ifndef VRPLY_H_INCLUDED
#define VRPLY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRGeometryPtr loadPly(string path);

OSG_END_NAMESPACE;

#endif // VRPLY_H_INCLUDED
