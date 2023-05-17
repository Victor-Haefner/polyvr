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

struct ocChunkRef {
    string path;
    size_t offset = 0;
    size_t size = 0;
    ofstream stream;
};

struct ocSerialNode {
    //double size = 0;
    //Vec3d center;
    size_t chunkOffset = 0;
    size_t chunkSize = 0;
    int children[8] = {0,0,0,0,0,0,0,0};
};

void loadE57(string path, VRTransformPtr res, map<string, string> importOptions);
void loadPCB(string path, VRTransformPtr res, map<string, string> importOptions);
void loadXYZ(string path, VRTransformPtr res, map<string, string> importOptions);

void writeE57(VRPointCloudPtr pcloud, string path);
void convertE57(string pathIn, string pathOut);

OSG_END_NAMESPACE;

#endif // E57_H_INCLUDED
