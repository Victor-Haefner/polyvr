#ifndef TEXTUREMANAGER_H
#define	TEXTUREMANAGER_H

#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGImage.h>

using namespace OSG;
using namespace std;

class TextureManager {
    private:
        TextureObjChunkRecPtr texStreetSegment;
        TextureObjChunkRecPtr texStreetJoint;
        TextureObjChunkRecPtr texSubQuad;
        vector<TextureObjChunkRecPtr> treeMapList;

        map<string, TextureObjChunkRecPtr> texMap;

    public:
        TextureManager();

        TextureObjChunkRecPtr getTexture(string key);

        vector<TextureObjChunkRecPtr> getTreeMap();
};

#endif	/* TEXTUREMANAGER_H */

