#include "VRTextureRenderer.h"
#include "core/objects/OSGObject.h"
#include "core/objects/object/OSGCore.h"
#include "core/objects/VRCamera.h"
#include "core/objects/OSGCamera.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scene/VRScene.h"
#include "core/scene/rendering/VRDefShading.h"
#include "core/math/boundingbox.h"

#include <OpenSG/OSGBackground.h>
#include <OpenSG/OSGSimpleStage.h>
#include <OpenSG/OSGFrameBufferObject.h>
#include <OpenSG/OSGTextureBuffer.h>
#include <OpenSG/OSGRenderBuffer.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTexGenChunk.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGSimpleTexturedMaterial.h>

#include <OpenSG/OSGGLUT.h>
#include <OpenSG/OSGGLUTWindow.h>
#include <OpenSG/OSGPassiveWindow.h>
#include <OpenSG/OSGViewport.h>
#include <OpenSG/OSGFBOViewport.h>
#include <OpenSG/OSGRenderAction.h>
#include <OpenSG/OSGSolidBackground.h>

#define GLSL(shader) #shader

using namespace OSG;

template<> string typeName(const VRTextureRenderer& o) { return "VRTextureRenderer"; }
template<> string typeName(const VRTextureRenderer::CHANNEL& o) { return "VRTextureRenderer::CHANNEL"; }

template<> int toValue(stringstream& ss, VRTextureRenderer::CHANNEL& e) {
    string s = ss.str();
    if (s == "RENDER") { e = VRTextureRenderer::RENDER; return true; }
    if (s == "DIFFUSE") { e = VRTextureRenderer::DIFFUSE; return true; }
    if (s == "NORMAL") { e = VRTextureRenderer::NORMAL; return true; }
    return false;
}

OSG_BEGIN_NAMESPACE;
struct VRTextureRenderer::Data {
    int fboWidth = 256;
    int fboHeight = 256;
    FrameBufferObjectRefPtr fbo;
    TextureObjChunkRefPtr   fboTex;
    ImageRefPtr             fboTexImg;
    TextureObjChunkRefPtr   fboDTex;
    ImageRefPtr             fboDTexImg;
    SimpleStageRefPtr       stage;

    // render once ressources
    RenderActionRefPtr ract;
    WindowMTRecPtr     win;
    ViewportMTRecPtr   view;
    VRDefShadingPtr deferredStage;
    VRObjectPtr deferredStageRoot;
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

    auto scene = VRScene::getCurrent();
    auto hlight = scene->getRoot()->find("Headlight");
    hlight->addChild( OSGObject::create(flagScene) );
}

VRTextureRenderer::VRTextureRenderer(string name) : VRObject(name) {
    data = new Data();

    data->fboTex = TextureObjChunk::create();
    data->fboTexImg = Image::create();
    data->fboTexImg->set(Image::OSG_RGB_PF, data->fboWidth, data->fboHeight);
    data->fboTex->setImage(data->fboTexImg);
    data->fboTex->setMinFilter(GL_NEAREST);
    data->fboTex->setMagFilter(GL_NEAREST);
    data->fboTex->setWrapS(GL_CLAMP_TO_EDGE);
    data->fboTex->setWrapT(GL_CLAMP_TO_EDGE);

    TextureBufferRefPtr texBuf = TextureBuffer::create();
    texBuf->setTexture(data->fboTex);

    data->fboDTexImg = Image::create();
    data->fboDTexImg->set(Image::OSG_RGB_PF, data->fboWidth, data->fboHeight);
    data->fboDTex = TextureObjChunk::create();
    data->fboDTex->setImage(data->fboDTexImg);
    data->fboDTex->setMinFilter(GL_NEAREST);
    data->fboDTex->setMagFilter(GL_NEAREST);
    data->fboDTex->setWrapS(GL_CLAMP_TO_EDGE);
    data->fboDTex->setWrapT(GL_CLAMP_TO_EDGE);
    data->fboDTex->setExternalFormat(GL_DEPTH_COMPONENT);
    data->fboDTex->setInternalFormat(GL_DEPTH_COMPONENT24); //24/32
    data->fboDTex->setCompareMode(GL_NONE);
    data->fboDTex->setCompareFunc(GL_LEQUAL);
    data->fboDTex->setDepthMode(GL_INTENSITY);
    TextureBufferRefPtr texDBuf = TextureBuffer::create();
    texDBuf->setTexture(data->fboDTex);

    RenderBufferRefPtr depthBuf = RenderBuffer::create();
    depthBuf->setInternalFormat(GL_DEPTH_COMPONENT24);

    data->fbo = FrameBufferObject::create();
    data->fbo->setColorAttachment(texBuf, 0);
    //data->fbo->setColorAttachment(texDBuf, 1);
    data->fbo->setDepthAttachment(texDBuf); //HERE depthBuf/texDBuf
    data->fbo->editMFDrawBuffers()->push_back(GL_DEPTH_ATTACHMENT_EXT);
    data->fbo->editMFDrawBuffers()->push_back(GL_COLOR_ATTACHMENT0_EXT);
    data->fbo->setWidth (data->fboWidth );
    data->fbo->setHeight(data->fboHeight);
    data->fbo->setPostProcessOnDeactivate(true);

    texBuf->setReadBack(true);
    texDBuf->setReadBack(true);

    mat = VRMaterial::create("VRTextureRenderer");
    mat->setTexture(data->fboTex, 0);
    mat->setTexture(data->fboDTex, 1);
    //mat->setShaderParameter<int>("tex0", 0); // TODO: will fail because shader not yet defined..
    //mat->setShaderParameter<int>("tex1", 1);

    auto scene = VRScene::getCurrent();

    // Stage
    data->stage = SimpleStage::create();
    data->stage->setRenderTarget(data->fbo);
    data->stage->setSize(0.0f, 0.0f, 1.0f, 1.0f);
    data->stage->setBackground( scene->getBackground() );

    setCore(OSGCore::create(data->stage), "TextureRenderer");

    // for deferred rendering
    data->deferredStageRoot = VRObject::create("TextureRendererDeferredRoot");
    data->deferredStage = VRDefShading::create();
    data->deferredStage->initDeferredShading(data->deferredStageRoot);
    data->deferredStage->setDeferredShading(true);
    data->deferredStage->setBackground( scene->getBackground() );
}

VRTextureRenderer::~VRTextureRenderer() { delete data; }
VRTextureRendererPtr VRTextureRenderer::create(string name) { return VRTextureRendererPtr( new VRTextureRenderer(name) ); }

void VRTextureRenderer::setBackground(Color3f c) {
    SolidBackgroundMTRecPtr bg = SolidBackground::create();
    bg->setAlpha(0);
    bg->setColor(c);
    mat->enableTransparency();
    data->stage->setBackground( bg );
    //if (data->deferredStage) data->deferredStage->setBackground( bg );
}

void VRTextureRenderer::setup(VRCameraPtr c, int width, int height, bool alpha) {
    cam = c;
    data->fboWidth = width;
    data->fboHeight = height;
    data->fbo->setWidth (data->fboWidth );
    data->fbo->setHeight(data->fboHeight);
    if (alpha) {
        data->fboTexImg->set(Image::OSG_RGBA_PF, data->fboWidth, data->fboHeight);
        data->fboDTexImg->set(Image::OSG_RGBA_PF, data->fboWidth, data->fboHeight);
    }
    else {
        data->fboTexImg->set(Image::OSG_RGB_PF, data->fboWidth, data->fboHeight);
        data->fboDTexImg->set(Image::OSG_RGB_PF, data->fboWidth, data->fboHeight);
    }
    data->stage->setCamera(cam->getCam()->cam);
    if (data->deferredStage) data->deferredStage->setDSCamera( cam->getCam() );
}

VRMaterialPtr VRTextureRenderer::getMaterial() { return mat; }
VRCameraPtr VRTextureRenderer::getCamera() { return cam; }

void VRTextureRenderer::setActive(bool b) {
    if (b) setCore(OSGCore::create(data->stage), "TextureRenderer", true);
    else setCore(OSGCore::create(Group::create()), "TextureRenderer", true);
}

void VRTextureRenderer::setChannelSubstitutes(CHANNEL c) {
    auto obj = getChild(0);
    if (obj) obj = obj->getLink(0); // TODO: this is comming from tree LODs, refactor please!
    if (!obj) return;
    for (auto geo : obj->getChildren(true, "Geometry")) {
        auto g = dynamic_pointer_cast<VRGeometry>(geo);
        auto m = g->getMaterial();
        if ( substitutes[c].count(m.get()) ) {
            auto sm = substitutes[c][m.get()];
            originalMaterials[sm.get()] = m;
            sm->setDeferred(0);
            cout << "    sub mat " << g->getName() << endl;
            g->setMaterial(sm);
        }
    }
}

void VRTextureRenderer::resetChannelSubstitutes() {
    auto obj = getChild(0);
    if (obj) obj = obj->getLink(0); // TODO: this is comming from tree LODs, refactor please!
    if (!obj) return;
    for (auto geo : obj->getChildren(true, "Geometry")) {
        auto g = dynamic_pointer_cast<VRGeometry>(geo);
        auto m = g->getMaterial();
        if (!originalMaterials.count(m.get())) continue;
        g->setMaterial(originalMaterials[m.get()]);
    }
    originalMaterials.clear();
}

void VRTextureRenderer::setMaterialSubstitutes(map<VRMaterial*, VRMaterialPtr> s, CHANNEL c) {
    substitutes[c] = s;
}

VRTexturePtr VRTextureRenderer::renderOnce(CHANNEL c) {
    if (!cam) return 0;

    bool deferred = VRScene::getCurrent()->getDefferedShading();

    if (!data->ract) {
        data->ract = RenderAction::create();
        if (deferred) {
            GLUTWindowRecPtr gwin = GLUTWindow::create();
            glutInitWindowSize(data->fboWidth, data->fboHeight);
            int winID = glutCreateWindow("PolyVR");
            gwin->setGlutId(winID);
            gwin->setSize(data->fboWidth, data->fboHeight);
            gwin->init();
            data->win = gwin;

            FBOViewportRecPtr fboView = FBOViewport::create();
            fboView->setFrameBufferObject(data->fbo); // replaces stage!
            fboView->setRoot(data->deferredStageRoot->getNode()->node);
            data->view = fboView;

            auto lights = VRScene::getCurrent()->getRoot()->getChildren(true, "Light");
            for (auto obj : lights) if (auto l = dynamic_pointer_cast<VRLight>(obj)) data->deferredStage->addDSLight(l);
            data->deferredStage->setDSCamera( cam->getCam() );
            for (auto link : getLinks()) data->deferredStageRoot->addLink(link);
            for (auto child : getChildren()) data->deferredStageRoot->addChild(child);
            clearLinks();
        } else {
            data->win = PassiveWindow::create();
            data->view = Viewport::create();
            data->view->setRoot(getNode()->node);
        }

        data->win->addPort(data->view);
        data->view->setSize(0, 0, 1, 1);
        data->view->setCamera(cam->getCam()->cam);
        data->view->setBackground(data->stage->getBackground());
    }

    if (c != RENDER) setChannelSubstitutes(c);
    data->win->render(data->ract);
    if (deferred) { // hack, TODO: for some reason the fbo gets not updated the first render call..
        data->win->render(data->ract);
    }
    ImageMTRecPtr img = Image::create();
    img->set( data->fboTexImg );
    if (c != RENDER) resetChannelSubstitutes();
    return VRTexture::create( img );
}

/** special setup
    - get all lights and light beacons above scene node and duplicate them
    - link scene node blow lights
    - render diffuse channel
    - render normal channel
*/

VRMaterialPtr VRTextureRenderer::createTextureLod(VRObjectPtr obj, PosePtr camP, int res, float aspect, float fov, Color3f bg) {
    VRObjectPtr tmpScene = VRObject::create("tmpScene");
    for (auto a : obj->getAncestry()) {
        if (a->getType() == "Light") {
            VRLightPtr l = dynamic_pointer_cast<VRLight>( a->duplicate() );
            tmpScene->addChild(l);
            tmpScene = l;

            auto b = dynamic_pointer_cast<VRLight>( a )->getBeacon();
            auto p = b->getWorldPose();
            auto lb = VRLightBeacon::create();
            l->setBeacon(lb);
            lb->setWorldPose(p);
        }
    }

    auto cam = VRCamera::create("cam", false); // segfault when threaded
    cam->setFov(fov); //0.33
    cam->setAspect(1);
    cam->setPose(camP);
    cam->updateChange();
    tmpScene->addChild(cam);
    tmpScene->addLink(obj);

    setBackground(bg);
    addChild(tmpScene);
	setup(cam, res, res/aspect, true);

	auto scene = VRScene::getCurrent();
	bool deferred = scene->getDefferedShading();
	if (deferred) scene->setDeferredShading(false);
    auto texDiffuse = renderOnce(DIFFUSE);
    auto texNormals = renderOnce(NORMAL);

    VRMaterialPtr mat = VRMaterial::create("lod");
    mat->setTexture(texDiffuse, false, 0);
    mat->setTexture(texNormals, false, 1);
    mat->setTextureParams(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_MODULATE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	if (deferred) scene->setDeferredShading(true);
    return mat;
}

#define protected public

#include <OpenSG/OSGCubeMapGenerator.h>
#include <OpenSG/OSGVisitSubTree.h>
#include <OpenSG/OSGConfig.h>

#include <iostream>

#include <OpenSG/OSGGLUT.h>
#include <OpenSG/OSGNode.h>

#include <OpenSG/OSGTrackball.h>
#include <OpenSG/OSGLine.h>
#include <OpenSG/OSGPerspectiveCamera.h>
#include <OpenSG/OSGTransform.h>
#include <OpenSG/OSGComponentTransform.h>
#include <OpenSG/OSGRenderAction.h>
#include <OpenSG/OSGWindow.h>
#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGSolidBackground.h>
#include <OpenSG/OSGSkyBackground.h>
#include <OpenSG/OSGGLUTWindow.h>
#include <OpenSG/OSGDirectionalLight.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGCubeMapGenerator.h>
#include <OpenSG/OSGImageFileHandler.h>
#include <OpenSG/OSGVisitSubTree.h>


OSG::NodeTransitPtr setupAnim(OSG::Node* lightBeacon) {
    OSG::NodeTransitPtr           dlight = OSG::Node::create();
    OSG::DirectionalLightUnrecPtr dl     = OSG::DirectionalLight::create();

    dlight->setCore(dl);
    dl->setAmbient( .3f, .3f, .3f, 1 );
    dl->setDiffuse( .8f, .8f, .8f, .8f );
    dl->setDirection(0,0,1);
    dl->setBeacon( lightBeacon);

    OSG::NodeRecPtr scene = makeNodeFor( OSG::Group::create() );
    dlight->addChild(scene);

    static const OSG::Real32 aOffsets[6][3] = {
        { -5.5,  0.0,  0.0 },
        {  5.5,  0.0,  0.0 },
        {  0.0, -5.5,  0.0 },
        {  0.0,  5.5,  0.0 },
        {  0.0,  0.0, -5.5 },
        {  0.0,  0.0,  5.5 }
    };

    static const OSG::Real32 aDiffuse[6][3] = {
        { 1.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f },
        { 0.f, 0.f, 1.f },
        { 1.f, 1.f, 0.f },
        { 1.f, 0.f, 1.f },
        { 0.f, 1.f, 1.f }
    };

    for(OSG::UInt32 i = 0; i < 6; ++i) {
        OSG::NodeUnrecPtr pTN          = OSG::Node::create();
        OSG::GeometryUnrecPtr pGeo     = OSG::makeBoxGeo(1.f, 1.f, 1.f, 2,   2,   2);
        OSG::NodeUnrecPtr     pGeoNode = OSG::Node::create();

        pGeoNode->setCore(pGeo);
        OSG::SimpleMaterialUnrecPtr pMat = OSG::SimpleMaterial::create();
        pMat->setDiffuse(OSG::Color3f(aDiffuse[i][0], aDiffuse[i][1], aDiffuse[i][2]));
        pMat->setAmbient(OSG::Color3f(aDiffuse[i][0], aDiffuse[i][1], aDiffuse[i][2]));
        pGeo->setMaterial(pMat);

        OSG::ComponentTransformRecPtr t = OSG::ComponentTransform::create();
        t->editTranslation().setValues(aOffsets[i][0], aOffsets[i][1], aOffsets[i][2]);

        pTN->setCore(t);
        pTN->addChild(pGeoNode);
        scene->addChild(pTN);
    }

    return dlight;
}

/*VRTexturePtr VRTextureRenderer::createCubeMap() {
    OSG::CubeMapGeneratorUnrecPtr pCubeGen = OSG::CubeMapGenerator::create();
    OSG::SolidBackgroundUnrecPtr cubeBkgnd = OSG::SolidBackground::create();
    cubeBkgnd->setColor(OSG::Color3f(0.5f, 0.3f, 0.3f));
    OSG::TransformRecPtr scene_trans = OSG::Transform::create();
    scene_trans->editMatrix()[3][2] = -9;
    OSG::NodeRecPtr pCubeRoot = makeNodeFor(pCubeGen);
    OSG::NodeRecPtr sceneTrN  = makeNodeFor(scene_trans);
    OSG::NodeRecPtr root = makeNodeFor(OSG::Group::create());
    root->addChild(sceneTrN);
    sceneTrN->addChild(pCubeRoot);
    pCubeRoot->addChild(OSG::makeSphere(4, 2.0));
    root->editVolume().setInfinite();
    root->editVolume().setStatic  ();
    OSG::NodeRecPtr pAnimRoot = setupAnim(root);
    pCubeGen->setRenderTarget(data->fbo);
    pCubeGen->setTextureFormat(GL_RGB32F_ARB );
    pCubeGen->setSize         (512, 512);
    pCubeGen->setTexUnit      (0);
    pCubeGen->setBackground(data->stage->getBackground());
    pCubeGen->setRoot(pAnimRoot);
    addChild(OSGObject::create(root));

    data->ract = RenderAction::create();
    data->win = PassiveWindow::create();
    data->view = Viewport::create();
    //data->view->setRoot(getNode()->node);
    data->view->setRoot(pCubeRoot);

    data->win->addPort(data->view);
    data->view->setSize(0, 0, 1, 1);
    data->view->setCamera(cam->getCam()->cam);
    data->view->setBackground(data->stage->getBackground());

    data->win->render(data->ract);
    ImageMTRecPtr img = Image::create();
    img->set( data->fboTexImg );
    return VRTexture::create( img );
}*/

vector<VRTexturePtr> VRTextureRenderer::createCubeMaps() {
    if (!cam) return {};
    auto pose = cam->getPose();
    auto p = cam->getFrom();

    cam->setTransform(p, Vec3d(0,0,-1), Vec3d(0,1,0));
    auto texFront = renderOnce();
    cam->setTransform(p, Vec3d(0,0,1), Vec3d(0,1,0));
    auto texBack = renderOnce();
    cam->setTransform(p, Vec3d(-1,0,0), Vec3d(0,1,0));
    auto texLeft = renderOnce();
    cam->setTransform(p, Vec3d(1,0,0), Vec3d(0,1,0));
    auto texRight = renderOnce();
    cam->setTransform(p, Vec3d(0,1,0), Vec3d(0,0,1));
    auto texUp = renderOnce();
    cam->setTransform(p, Vec3d(0,-1,0), Vec3d(0,0,-1));
    auto texDown = renderOnce();

    cam->setPose(pose);
    return {texFront, texBack, texLeft, texRight, texUp, texDown};
}
