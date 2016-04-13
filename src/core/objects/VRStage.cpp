#include "VRStage.h"
#include <OpenSG/OSGSolidBackground.h>
#include <OpenSG/OSGTextureBuffer.h>
#include <OpenSG/OSGRenderBuffer.h>

#include "core/objects/VRCamera.h"
#include "core/objects/material/VRMaterial.h"

using namespace OSG;

VRObjectPtr VRStage::copy(vector<VRObjectPtr> children) {
    VRStagePtr g = VRStage::create(getBaseName());
    return g;
}

VRStage::VRStage(string name) : VRObject(name) {
    initStage();
}

VRStage::~VRStage() {}

VRStagePtr VRStage::create(string name) { return shared_ptr<VRStage>(new VRStage(name) ); }
VRStagePtr VRStage::ptr() { return static_pointer_cast<VRStage>( shared_from_this() ); }

void VRStage::setActive(bool b) {
    active = b;
    if (b) setCore(stage, "Stage", true);
    else setCore(Group::create(), "Object", true);
}

bool VRStage::isActive() { return active; }

void VRStage::initStage() {
    stage = SimpleStage::create();
    setCore(stage, "Stage");
    stage->setSize(0.0f, 0.0f, 1.0f, 1.0f);

    SolidBackgroundRefPtr gb = SolidBackground::create();
    gb->setColor( Color3f(1,1,1) );
    stage->setBackground(gb);
}

void VRStage::initFBO() {
    fbo = FrameBufferObject::create();
    fbo->editMFDrawBuffers()->push_back(GL_COLOR_ATTACHMENT0_EXT);
    fbo->setPostProcessOnDeactivate(true);
    stage->setRenderTarget(fbo);

    // color buffer
    fboImg = Image::create();
    fboTex = TextureObjChunk::create();
    fboTex->setImage(fboImg);
    TextureBufferRefPtr texBuf = TextureBuffer::create();
    texBuf->setTexture(fboTex);
    fbo->setColorAttachment(texBuf, 0);
    target->setTexture(fboTex, tex_id);

    // depth buffer
    RenderBufferRefPtr depthBuf = RenderBuffer::create();
    depthBuf->setInternalFormat(GL_DEPTH_COMPONENT32); // 16 24 32
    fbo->setDepthAttachment(depthBuf);
}

void VRStage::update() {
    if (target) {
        if (!fboTex) initFBO();
        fboImg->set(Image::OSG_RGBA_PF, size[0], size[1]);
    }

    if (fbo) {
        fbo->setWidth (size[0]);
        fbo->setHeight(size[1]);
    }
}

void VRStage::setTarget(VRMaterialPtr mat, int tid) {
    target = mat; tex_id = tid;
    update();
}

void VRStage::setSize( Vec2i s ) { size = s; update(); }
void VRStage::setCamera(VRCameraPtr cam) { stage->setCamera( cam->getCam() ); }
