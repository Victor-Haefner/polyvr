#ifndef VRCOLLADA_H_INCLUDED
#define VRCOLLADA_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObject;

VRObject* loadCollada(string path, VRObject* objects);

OSG_END_NAMESPACE;


#endif // VRCOLLADA_H_INCLUDED

