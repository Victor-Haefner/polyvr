#ifndef VRDXF_H_INCLUDED
#define VRDXF_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGeometry;

VRGeometry* loadDXF(string path);

OSG_END_NAMESPACE;

#endif // VRDXF_H_INCLUDED
