#include "VRTextureRenderer.h"
#include "core/objects/VRCamera.h"
#include "core/objects/material/VRMaterial.h"

#include <OpenSG/OSGSimpleStage.h>
#include <OpenSG/OSGFrameBufferObject.h>
#include <OpenSG/OSGTextureBuffer.h>
#include <OpenSG/OSGRenderBuffer.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTexGenChunk.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

struct VRTextureRenderer::Data {
    int fboWidth = 256;
    int fboHeight = 256;
    FrameBufferObjectRefPtr fbo;
    ImageRefPtr             fboTexImg;
    SimpleStageRefPtr stage;
};

VRTextureRenderer::VRTextureRenderer(string name) : VRObject(name) {
    data = new Data();

    // FBO
    TextureObjChunkRefPtr fboTex = TextureObjChunk::create();
    TextureBufferRefPtr texBuf = TextureBuffer::create();
    RenderBufferRefPtr depthBuf = RenderBuffer::create();
    data->fbo = FrameBufferObject::create();
    data->fboTexImg = Image::create();

    data->fboTexImg->set(Image::OSG_RGB_PF, data->fboWidth, data->fboHeight);
    fboTex->setImage(data->fboTexImg);
    texBuf->setTexture(fboTex);
    depthBuf->setInternalFormat(GL_DEPTH_COMPONENT24);

    data->fbo->setColorAttachment(texBuf, 0);
    data->fbo->setDepthAttachment(depthBuf);
    data->fbo->editMFDrawBuffers()->push_back(GL_COLOR_ATTACHMENT0_EXT);
    data->fbo->setWidth (data->fboWidth );
    data->fbo->setHeight(data->fboHeight);
    data->fbo->setPostProcessOnDeactivate(true);

    mat = new VRMaterial("VRTextureRenderer");
    mat->setTexture(data->fboTexImg);

    // Stage
    data->stage = SimpleStage::create();
    data->stage->setRenderTarget(data->fbo);
    data->stage->setSize(0.0f, 0.0f, 1.0f, 1.0f);

    setCore(data->stage, "TextureRenderer");
}

VRTextureRenderer::~VRTextureRenderer() {
    delete data;
}

void VRTextureRenderer::setup(VRCamera* cam, int width, int height) {
    data->fboWidth = width;
    data->fboHeight = height;
    data->fbo->setWidth (data->fboWidth );
    data->fbo->setHeight(data->fboHeight);
    data->stage->setCamera(cam->getCam());
}

VRMaterial* VRTextureRenderer::getMaterial() { return mat; }

OSG_END_NAMESPACE;
