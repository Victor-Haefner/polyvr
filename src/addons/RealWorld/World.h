#ifndef WORLD_H
#define	WORLD_H

#include "MapData.h"
#include "core/objects/VRObjectFwd.h"
#include <map>
//#include <OpenSG/OSGTextureObjChunk.h>

class TextureManager;

OSG_BEGIN_NAMESPACE;
using namespace std;

class World {
    private:
        /*TextureObjChunkRecPtr texStreetSegment;
        TextureObjChunkRecPtr texStreetJoint;
        TextureObjChunkRecPtr texSubQuad;
        vector<TextureObjChunkRecPtr> treeMapList;
        map<string, TextureObjChunkRecPtr> texMap;*/

    public:
        map<string, VRGeometryPtr> meshes;
        map<string, StreetJoint*> streetJoints;
        map<string, StreetSegment*> streetSegments;
        map<string, Building*> buildings;

        vector<string> listUnloadJointMeshes;
        vector<string> listLoadJointMeshes;

        vector<string> listUnloadSegmentMeshes;
        vector<string> listLoadSegmentMeshes;

        vector<string> listUnloadBuildingMeshes;
        vector<string> listLoadBuildingMeshes;

        vector<MapData*> listUnloadAreaMeshes;
        vector<MapData*> listLoadAreaMeshes;

        World();

        void updateGeometry();

        //TextureObjChunkRecPtr getTexture(string key);
        //vector<TextureObjChunkRecPtr> getTreeMap();
};

OSG_END_NAMESPACE;

#endif	/* WORLD_H */

