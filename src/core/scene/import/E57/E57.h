#ifndef E57_H_INCLUDED
#define E57_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <string>
#include <map>
#include <fstream>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void loadE57(string path, VRTransformPtr res, map<string, string> importOptions);
void loadPCB(string path, VRTransformPtr res, map<string, string> importOptions);
void loadXYZ(string path, VRTransformPtr res, map<string, string> importOptions);

void writeE57(VRPointCloudPtr pcloud, string path);
void convertE57(vector<string> pathsIn, string pathOut);

OSG_END_NAMESPACE;

#endif // E57_H_INCLUDED
