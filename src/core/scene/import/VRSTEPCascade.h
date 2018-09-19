#ifndef VRSTEPCASCADE_H_INCLUDED
#define VRSTEPCASCADE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void loadSTEPCascade(string path, VRTransformPtr res);

OSG_END_NAMESPACE;

#endif // VRSTEPCASCADE_H_INCLUDED
