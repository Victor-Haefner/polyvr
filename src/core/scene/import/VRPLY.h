#ifndef VRPLY_H_INCLUDED
#define VRPLY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void loadPly(string path, VRTransformPtr res);
void writePly(VRGeometryPtr geo, string path);

OSG_END_NAMESPACE;

#endif // VRPLY_H_INCLUDED
