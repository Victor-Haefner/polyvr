#ifndef VRTEXTUREMOSAIC_H_INCLUDED
#define VRTEXTUREMOSAIC_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include "core/objects/VRObjectFwd.h"
#include "core/objects/material/VRTexture.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRTextureMosaic : public VRTexture {
    private:
        struct Entry {
            VRTexturePtr tex;
            Vec2i pos;
            Vec2f uv;
        };

    public:
        VRTextureMosaic();
        ~VRTextureMosaic();

        static VRTextureMosaicPtr create();
        VRTextureMosaicPtr ptr();

        void add(VRTexturePtr tex, Vec2i pos);
};

OSG_END_NAMESPACE;


#endif // VRTEXTUREMOSAIC_H_INCLUDED
