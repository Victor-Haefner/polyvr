#ifndef VRTEXTURERENDERER_H_INCLUDED
#define VRTEXTURERENDERER_H_INCLUDED

#include <OpenSG/OSGColor.h>

#include "core/objects/VRObjectFwd.h"
#include "core/tools/VRToolsFwd.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/material/VRMaterialFwd.h"
#include "core/networking/VRNetworkingFwd.h"
#include "core/utils/VRMutex.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRTextureRenderer : public VRObject {
    public:
        enum CHANNEL {
            RENDER = 0,
            DIFFUSE = 1,
            NORMAL = 2
        };

    private:
        struct Data;
        Data* data = 0;
        VRMaterialPtr mat = 0;
        VRTexturePtr fbotex = 0;
        VRCameraPtr cam = 0;
        VRTCPServerPtr server;
        VRUpdateCbPtr updateCb;
        VRMutex mtx;

        void setChannelFP(string fp);
        void resetChannelFP();

        map<CHANNEL, map<VRMaterial*, VRMaterialPtr>> substitutes;
        map<VRMaterial*, VRMaterialPtr> originalMaterials;

        void setChannelSubstitutes(CHANNEL c);
        void resetChannelSubstitutes();

        string serverCallback(string data);
        void prepareTextureForStream();

    public:
        VRTextureRenderer(string name, bool readback = false);
        ~VRTextureRenderer();

        static VRTextureRendererPtr create(string name = "textureRenderer", bool readback = false);

        void setup(VRCameraPtr cam, int width, int height, bool alpha = false);
        void setReadback(bool RGBReadback, bool depthReadback);
        void setCam(VRCameraPtr c);
        void setStageCam(OSGCameraPtr cam);
        void setMaterialSubstitutes(map<VRMaterial*, VRMaterialPtr> substitutes, CHANNEL c);
        void setBackground(Color3f c, float a = 0);
        void updateBackground();

        void setActive(bool b);
        VRMaterialPtr getMaterial();
        VRMaterialPtr copyMaterial();
        VRCameraPtr getCamera();
        Vec2i getResolution();

        VRTexturePtr renderOnce(CHANNEL c = RENDER);
        vector<VRTexturePtr> createCubeMaps(VRTransformPtr focusObject);
        VRMaterialPtr createTextureLod(VRObjectPtr scene, PosePtr cam, int res, float aspect, float fov, Color3f bg);

        void exportImage(string path);
        void exportDepthImage(string path);
        string startServer(int port);
        void stopServer();

        static void updateSceneBackground();

        void test();
};

OSG_END_NAMESPACE;

#endif // VRTEXTURERENDERER_H_INCLUDED
