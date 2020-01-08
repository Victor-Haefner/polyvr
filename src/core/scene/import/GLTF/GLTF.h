#ifndef GLTF_H_INCLUDED
#define GLTF_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRUtilsFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void loadGLTF(string path, VRTransformPtr res, VRProgressPtr p, bool thread = false);
void writeGLTF(VRObjectPtr res, string path);

OSG_END_NAMESPACE;

#endif // GLTF_H_INCLUDED
