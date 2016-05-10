#ifndef VRVTK_H_INCLUDED
#define VRVTK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void loadVtk(string path, VRTransformPtr res);

OSG_END_NAMESPACE;

#endif // VRVTK_H_INCLUDED
