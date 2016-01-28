#ifndef VRTEXTURERENDERER_H_INCLUDED
#define VRTEXTURERENDERER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/objects/object/VRObject.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRTextureRenderer : public VRObject {
    private:
        struct Data;
        Data* data = 0;
        VRMaterialPtr mat = 0;
        VRTexturePtr fbotex = 0;

    public:
        VRTextureRenderer(string name);
        ~VRTextureRenderer();

        static VRTextureRendererPtr create(string name);

        void setup(VRCameraPtr cam, int width, int height);

        VRMaterialPtr getMaterial();
};

OSG_END_NAMESPACE;

#endif // VRTEXTURERENDERER_H_INCLUDED
