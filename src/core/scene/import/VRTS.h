#ifndef VRTS_H_INCLUDED
#define VRTS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void loadTS(string path, VRTransformPtr res, map<string, string> options);
void writeTS(VRGeometryPtr geo, string path);

OSG_END_NAMESPACE;

#endif // VRTS_H_INCLUDED
