#ifndef VRTEXTUREMOSAIC_H_INCLUDED
#define VRTEXTUREMOSAIC_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/math/OSGMathFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "core/objects/material/VRTexture.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRTextureMosaic : public VRTexture {
    private:
        struct Entry {
            VRTexturePtr tex;
            Vec2i pos;
            Vec2i size;
            Vec2f uv;
        };

        map<Vec2i, Entry> entries;

    public:
        VRTextureMosaic();
        ~VRTextureMosaic();

        static VRTextureMosaicPtr create();
        VRTextureMosaicPtr ptr();

        void add(VRTexturePtr tex, Vec2i pos, Vec2i ID);

        Vec2i getChunkPosition(Vec2i ID);
        Vec2i getChunkSize(Vec2i ID);
        Vec4d getChunkUV(Vec2i ID);
        vector<Vec2d> getChunkUVasVector(Vec2i ID);
};

OSG_END_NAMESPACE;


#endif // VRTEXTUREMOSAIC_H_INCLUDED
