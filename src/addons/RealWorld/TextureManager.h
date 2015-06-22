#ifndef TEXTUREMANAGER_H
#define	TEXTUREMANAGER_H

#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGImage.h>

using namespace OSG;
using namespace std;

namespace realworld {
    class TextureManager {
        public:
            TextureObjChunkRecPtr texStreetSegment;
            TextureObjChunkRecPtr texStreetJoint;
            TextureObjChunkRecPtr texSubQuad;
            vector<TextureObjChunkRecPtr> treeMapList;

            map<string, TextureObjChunkRecPtr> texMap;

            TextureObjChunkRecPtr getTexture(string key) {
                if (texMap.count(key)) return texMap[key];

                ImageRecPtr image = Image::create();
                image->read(("world/textures/"+key).c_str());
                TextureObjChunkRecPtr tex = TextureObjChunk::create();
                tex->setImage(image);
                texMap[key] = tex;
                return tex;
            }

            TextureManager() {
                ImageRecPtr imageStreetSegment = Image::create();
                imageStreetSegment->read("world/textures/street1.png");
                texStreetSegment = TextureObjChunk::create();
                texStreetSegment->setImage(imageStreetSegment);

                ImageRecPtr imageStreetJoint = Image::create();
                imageStreetJoint->read("world/textures/street1.png");
                texStreetJoint = TextureObjChunk::create();
                texStreetJoint->setImage(imageStreetJoint);

                ImageRecPtr imageSubQuad = Image::create();
                imageSubQuad->read("world/textures/asphalt.jpg");
                texSubQuad = TextureObjChunk::create();
                texSubQuad->setImage(imageSubQuad);

//                ImageRecPtr img = Image::create();
//                UChar8 data[] = {0,0,0, 255,50,50, 100,255,100, 255,255,255};
//                img->set( Image::OSG_RGB_PF, 2, 2, 1, 1, 1, 0, data);
//                texSubQuad = TextureObjChunk::create();
//                texSubQuad->setImage(img);

                for(int i = 0; i<4; i++){
                    TextureObjChunkRecPtr texTree;
                    ImageRecPtr imageTree = Image::create();
                    texTree = TextureObjChunk::create();
                    stringstream ss;
                    ss << i;
                    string str = ss.str();
                    imageTree->read(("world/textures/Tree/tree"+str+".png").c_str());
                    texTree->setImage(imageTree);
                    treeMapList.push_back(texTree);
                }




//                chunk->setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
//                chunk->setMagFilter( GL_LINEAR );
//                chunk->setWrapS( GL_CLAMP );//GL_CLAMP //GL_REPEAT
//                chunk->setWrapT( GL_CLAMP_TO_EDGE );//GL_CLAMP_TO_EDGE //GL_REPEAT
            }
    };
}

#endif	/* TEXTUREMANAGER_H */

