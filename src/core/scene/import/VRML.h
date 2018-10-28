#ifndef VRML_H_INCLUDED
#define VRML_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRUtilsFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void loadVRML(string path, VRTransformPtr res, VRProgressPtr p, bool thread = false);

OSG_END_NAMESPACE;

#endif // VRML_H_INCLUDED
