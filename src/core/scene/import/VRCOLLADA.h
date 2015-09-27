#ifndef VRCOLLADA_H_INCLUDED
#define VRCOLLADA_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRObjectPtr loadCollada(string path, VRObjectPtr objects);

OSG_END_NAMESPACE;


#endif // VRCOLLADA_H_INCLUDED

