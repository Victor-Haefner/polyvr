#ifndef VRTEXTURERENDERER_H_INCLUDED
#define VRTEXTURERENDERER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/object/VRObject.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRCamera;
class VRMaterial;

class VRTextureRenderer : public VRObject {
    private:
        struct Data;
        Data* data = 0;
        VRMaterial* mat = 0;

    public:
        VRTextureRenderer(string name);
        ~VRTextureRenderer();

        void setup(VRCamera* cam, int width, int height);

        VRMaterial* getMaterial();
};

OSG_END_NAMESPACE;

#endif // VRTEXTURERENDERER_H_INCLUDED
