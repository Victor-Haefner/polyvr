#include "VRTextureRenderer.h"
#include "core/objects/VRCamera.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"

#include <OpenSG/OSGSimpleStage.h>
#include <OpenSG/OSGFrameBufferObject.h>
#include <OpenSG/OSGTextureBuffer.h>
#include <OpenSG/OSGRenderBuffer.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTexGenChunk.h>


// tmp
#include <OpenSG/OSGGLUT.h>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGGLUTWindow.h>
#include <OpenSG/OSGSimpleSceneManager.h>
#include <OpenSG/OSGBaseFunctions.h>
#include <OpenSG/OSGTransform.h>
#include <OpenSG/OSGComponentTransform.h>
#include <OpenSG/OSGSimpleStage.h>
#include <OpenSG/OSGPointLight.h>
#include <OpenSG/OSGFrameBufferObject.h>
#include <OpenSG/OSGTextureBuffer.h>
#include <OpenSG/OSGRenderBuffer.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTexGenChunk.h>
#include <OpenSG/OSGTwoSidedLightingChunk.h>
#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGImageFunctions.h>
#include <OpenSG/OSGSimpleTexturedMaterial.h>
#include <OpenSG/OSGTextureTransformChunk.h>
#include <OpenSG/OSGGradientBackground.h>
#include <OpenSG/OSGPerspectiveCamera.h>
#include <OpenSG/OSGVisitSubTree.h>

using namespace std;
using namespace OSG;

OSG_BEGIN_NAMESPACE;
struct VRTextureRenderer::Data {
    int fboWidth = 256;
    int fboHeight = 256;
    FrameBufferObjectRefPtr fbo;
    TextureObjChunkRefPtr   fboTex;
    ImageRefPtr             fboTexImg;
    SimpleStageRefPtr stage;
    map<VRObject*, NodeRecPtr> links;
};
OSG_END_NAMESPACE;

void VRTextureRenderer::test() {
    NodeRefPtr     flagScene = makeCoredNode<Group>();
    GeometryRefPtr flagGeo   = makePlaneGeo(4, 2, 1, 1);
    flagGeo->setDlistCache(false);

    SimpleMaterialRefPtr  flagMat   = SimpleMaterial ::create();
    TextureEnvChunkRefPtr fboTexEnv = TextureEnvChunk::create();

    fboTexEnv->setEnvMode(GL_REPLACE);
    flagMat->addChunk(data->fboTex);
    flagMat->addChunk(fboTexEnv);
    flagGeo->setMaterial(flagMat);
    flagScene->addChild(NodeRefPtr(makeNodeFor(flagGeo)));

    auto scene = VRSceneManager::getCurrent();
    auto hlight = scene->getRoot()->find("Headlight");
    hlight->addChild( flagScene );
}

VRTextureRenderer::VRTextureRenderer(string name) : VRObject(name) {
    data = new Data();

    // FBO
    data->fboTex = TextureObjChunk::create();
    TextureBufferRefPtr texBuf = TextureBuffer::create();
    RenderBufferRefPtr depthBuf = RenderBuffer::create();
    data->fbo = FrameBufferObject::create();
    data->fboTexImg = Image::create();

    data->fboTexImg->set(Image::OSG_RGB_PF, data->fboWidth, data->fboHeight);
    data->fboTex->setImage(data->fboTexImg);
    texBuf->setTexture(data->fboTex);
    depthBuf->setInternalFormat(GL_DEPTH_COMPONENT24);

    data->fbo->setColorAttachment(texBuf, 0);
    data->fbo->setDepthAttachment(depthBuf);
    data->fbo->editMFDrawBuffers()->push_back(GL_COLOR_ATTACHMENT0_EXT);
    data->fbo->setWidth (data->fboWidth );
    data->fbo->setHeight(data->fboHeight);
    data->fbo->setPostProcessOnDeactivate(true);

    mat = VRMaterial::create("VRTextureRenderer");
    mat->setTexture(data->fboTex);

    auto scene = VRSceneManager::getCurrent();

    // Stage
    data->stage = SimpleStage::create();
    data->stage->setRenderTarget(data->fbo);
    data->stage->setSize(0.0f, 0.0f, 1.0f, 1.0f);
    data->stage->setBackground( scene->getBackground() );

    setCore(data->stage, "TextureRenderer");
}

VRTextureRenderer::~VRTextureRenderer() { delete data; }
VRTextureRendererPtr VRTextureRenderer::create(string name) { return VRTextureRendererPtr( new VRTextureRenderer(name) ); }

void VRTextureRenderer::setup(VRCameraPtr cam, int width, int height) {
    data->fboWidth = width;
    data->fboHeight = height;
    data->fbo->setWidth (data->fboWidth );
    data->fbo->setHeight(data->fboHeight);
    data->fboTexImg->set(Image::OSG_RGB_PF, data->fboWidth, data->fboHeight);
    data->stage->setCamera(cam->getCam());

    //test();
}

void VRTextureRenderer::addLink(VRObjectPtr obj) {
    if (data->links.count(obj.get())) return;

    VisitSubTreeRecPtr visitor = VisitSubTree::create();
    visitor->setSubTreeRoot(obj->getNode());
    NodeRecPtr visit_node = makeNodeFor(visitor);
    addChild(visit_node);

    data->links[obj.get()] = visit_node;
}

void VRTextureRenderer::remLink(VRObjectPtr obj) {
    if (!data->links.count(obj.get())) return;

    NodeRecPtr node = data->links[obj.get()];
    subChild(node);
    data->links.erase(obj.get());
}

VRMaterialPtr VRTextureRenderer::getMaterial() { return mat; }
