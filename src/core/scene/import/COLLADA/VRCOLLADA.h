#ifndef VRCOLLADA_H_INCLUDED
#define VRCOLLADA_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void loadCollada(string path, VRTransformPtr root, map<string, string> options);
void writeCollada(VRObjectPtr root, string path, map<string, string> options);

OSG_END_NAMESPACE;


#endif // VRCOLLADA_H_INCLUDED

