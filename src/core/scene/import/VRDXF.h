#ifndef VRDXF_H_INCLUDED
#define VRDXF_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void loadDXF(string path, VRTransformPtr res);

OSG_END_NAMESPACE;

#endif // VRDXF_H_INCLUDED
