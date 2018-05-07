#ifndef VRIFC_H_INCLUDED
#define VRIFC_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void loadIFC(string path, VRTransformPtr res);

OSG_END_NAMESPACE;

#endif // VRIFC_H_INCLUDED
