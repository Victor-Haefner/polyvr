#ifndef VRSTEPCASCADE_H_INCLUDED
#define VRSTEPCASCADE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <string>
#include <TopoDS_Shape.hxx>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRTransformPtr convertSTEPShape(const TopoDS_Shape& shape, bool subParts = false, bool relative_deflection = true, double linear_deflection = 0.1, double angular_deflection = 0.5);

void loadSTEPCascade(string path, VRTransformPtr res, map<string, string> options);

OSG_END_NAMESPACE;

#endif // VRSTEPCASCADE_H_INCLUDED
