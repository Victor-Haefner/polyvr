#ifndef VRPLY_H_INCLUDED
#define VRPLY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGeometry;

VRGeometry* loadPly(string path);

OSG_END_NAMESPACE;

#endif // VRPLY_H_INCLUDED
