#ifndef VRTEXTURERENDERER_H_INCLUDED
#define VRTEXTURERENDERER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/tools/VRToolsFwd.h"
#include "core/objects/object/VRObject.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRTextureRenderer : public VRObject {
    private:
        struct Data;
        Data* data = 0;
        VRMaterialPtr mat = 0;
        VRTexturePtr fbotex = 0;
        VRCameraPtr cam = 0;

        void test();

    public:
        VRTextureRenderer(string name);
        ~VRTextureRenderer();

        static VRTextureRendererPtr create(string name);

        void setup(VRCameraPtr cam, int width, int height, bool alpha = false);

        void setActive(bool b);
        VRMaterialPtr getMaterial();

        VRTexturePtr renderOnce();
};

OSG_END_NAMESPACE;

#endif // VRTEXTURERENDERER_H_INCLUDED
