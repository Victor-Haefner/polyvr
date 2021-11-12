#ifndef VRCOLLADA_H_INCLUDED
#define VRCOLLADA_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void loadCollada(string path, VRObjectPtr root, map<string, string> options);

OSG_END_NAMESPACE;


#endif // VRCOLLADA_H_INCLUDED

