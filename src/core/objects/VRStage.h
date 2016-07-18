#ifndef VRSTAGE_H_INCLUDED
#define VRSTAGE_H_INCLUDED

#include <OpenSG/OSGImage.h>
#include <OpenSG/OSGSimpleStage.h>
#include <OpenSG/OSGFrameBufferObject.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGProjectionCameraDecorator.h>

#include "core/objects/object/VRObject.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRStage : public VRObject {
    private:
        VRMaterialPtr target;
        ImageRecPtr fboImg;
        TextureObjChunkRefPtr fboTex;
        FrameBufferObjectRefPtr fbo;
        SimpleStageRefPtr stage;
        Vec2i size = Vec2i(512, 512);
        int tex_id = 0;
        bool active = false;

        void initStage();
        void initFBO();
        void setup();

    protected:
        VRObjectPtr copy(vector<VRObjectPtr> children);

    public:
        VRStage(string name);
        ~VRStage();

        static VRStagePtr create(string name = "None");
        VRStagePtr ptr();

        void setActive(bool b);
        bool isActive();

        void setSize( Vec2i size);
        void setTarget(VRMaterialPtr mat, int tid = 0);
        void setCamera(VRCameraPtr cam);
        void setCamera(ProjectionCameraDecoratorRecPtr cam);
        void setBackground(BackgroundRecPtr bg);
        void update();
};

OSG_END_NAMESPACE;

#endif // VRSTAGE_H_INCLUDED
