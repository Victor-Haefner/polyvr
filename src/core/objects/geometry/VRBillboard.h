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
        VRTexturePtr BBtexture;
        GeometryMTRecPtr BBplane;
        VRMaterialPtr BBmat;
        TextureObjChunkMTRecPtr BBtexChunk;
        VRShader* bbs;

        void initBBMesh(bool alpha);

        void updateBBTexture();

        void updateSize();

    public:
        VRBillboard(string name = "None", bool alpha = true);
        ~VRBillboard();

        static VRBillboardPtr create(string name = "None", bool alpha = true);
        VRBillboardPtr ptr();

        void setTexture(VRTexturePtr img);
        void setSize(float sizeW, float sizeH);

        static void createTestScene();
};

OSG_END_NAMESPACE;

#endif // VRBILLBOARD_H_INCLUDED
