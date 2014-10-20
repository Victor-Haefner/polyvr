#ifndef VRBILLBOARD_H_INCLUDED
#define VRBILLBOARD_H_INCLUDED

#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSimpleTexturedMaterial.h>
#include "VRGeometry.h"


OSG_BEGIN_NAMESPACE;
using namespace std;

class VRShader;

class VRBillboard : public VRGeometry {
    private:
        float BBsizeH;
        float BBsizeW;
        ImageRecPtr BBtexture;
        GeometryRecPtr BBplane;
        ChunkMaterialRecPtr BBmat;
        TextureObjChunkRecPtr BBtexChunk;
        VRShader* bbs;

        void initBBMesh(bool alpha);

        void updateBBTexture();

        void updateSize();

    public:
        VRBillboard(string name = "", bool alpha = true);

        ~VRBillboard();

        void setTexture(ImageRecPtr img);

        void setSize(float sizeW, float sizeH);

        static void createTestScene();
};

OSG_END_NAMESPACE;

#endif // VRBILLBOARD_H_INCLUDED
