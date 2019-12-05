#ifndef E57_H_INCLUDED
#define E57_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void loadE57(string path, VRTransformPtr res, map<string, string> importOptions);
void loadXYZ(string path, VRTransformPtr res, map<string, string> importOptions);
//void writeE57(VRGeometryPtr geo, string path);

OSG_END_NAMESPACE;

#endif // E57_H_INCLUDED
