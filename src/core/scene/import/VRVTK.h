#ifndef VRVTK_H_INCLUDED
#define VRVTK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void loadVTK(string path, VRTransformPtr res);
void writeVTK(VRObjectPtr obj, string path, map<string, string> options);

OSG_END_NAMESPACE;

#endif // VRVTK_H_INCLUDED
