#include <OpenSG/OSGGLUT.h>
#include <OpenSG/OSGGLUTWindow.h>

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
#include "core/objects/geometry/VRSky.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRGtkWindow.h"
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

#include <OpenSG/OSGPassiveWindow.h>
#include <OpenSG/OSGViewport.h>
#include <OpenSG/OSGFBOViewport.h>
#include <OpenSG/OSGRenderAction.h>
#include <OpenSG/OSGSolidBackground.h>

#define GLSL(shader) #shader

using namespace OSG;

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
#ifndef WITHOUT_DEFERRED_RENDERING
    VRDefShadingPtr deferredStage;
    VRObjectPtr deferredStageRoot;
#endif
};
OSG_END_NAMESPACE;

vector<VRTextureRendererWeakPtr> TRinstances;

void VRTextureRenderer::test() {
    return;
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
    auto hlight = scene->getRoot()->find("light");
    hlight->addChild( OSGObject::create(flagScene) );
}

VRTextureRenderer::VRTextureRenderer(string name, bool readback) : VRObject(name) {
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
    data->fbo->setDepthAttachment(texDBuf);
    data->fbo->editMFDrawBuffers()->push_back(GL_DEPTH_ATTACHMENT_EXT);
    data->fbo->editMFDrawBuffers()->push_back(GL_COLOR_ATTACHMENT0_EXT);
    data->fbo->setWidth (data->fboWidth );
    data->fbo->setHeight(data->fboHeight);
    data->fbo->setPostProcessOnDeactivate(true);

    if (readback) {
        texBuf->setReadBack(true);
        texDBuf->setReadBack(true);
    }

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

    setCore(OSGCore::create(data->stage), "TextureRenderer");

    // for deferred rendering
#ifndef WITHOUT_DEFERRED_RENDERING
    data->deferredStageRoot = VRObject::create("TextureRendererDeferredRoot");
    data->deferredStage = VRDefShading::create();
    data->deferredStage->initDeferredShading(data->deferredStageRoot);
    data->deferredStage->setDeferredShading(true);
#endif

    updateBackground();
}

VRTextureRenderer::~VRTextureRenderer() { delete data; }

VRTextureRendererPtr VRTextureRenderer::create(string name, bool readback) {
    auto tg = VRTextureRendererPtr( new VRTextureRenderer(name, readback) );
    TRinstances.push_back(tg);
    return tg;
}

void VRTextureRenderer::updateSceneBackground() {
    for (auto i : TRinstances) {
        auto tg = i.lock();
        if (tg) tg->updateBackground();
    }
}

void VRTextureRenderer::setBackground(Color3f c, float a) {
    SolidBackgroundRecPtr bg = SolidBackground::create();
    bg->setAlpha(a);
    bg->setColor(c);
    mat->enableTransparency();
    data->stage->setBackground( bg );
#ifndef WITHOUT_DEFERRED_RENDERING
    if (data->deferredStage) data->deferredStage->setBackground( bg );
#endif
    auto scene = VRScene::getCurrent();
    if (auto sky = scene->getSky()) remLink(sky);
}

void VRTextureRenderer::updateBackground() {
    auto scene = VRScene::getCurrent();

    data->stage->setBackground( scene->getBackground() );
#ifndef WITHOUT_DEFERRED_RENDERING
    if (data->deferredStage) data->deferredStage->setBackground( scene->getBackground() );
#endif

    if (auto sky = scene->getSky()) addLink(sky);
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
#ifndef WITHOUT_DEFERRED_RENDERING
    if (data->deferredStage) data->deferredStage->setDSCamera( cam->getCam() );
#endif
}

void VRTextureRenderer::setStageCam(OSGCameraPtr cam) { data->stage->setCamera(cam->cam); }
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
#ifndef WITHOUT_DEFERRED_RENDERING
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
            data->view->setBackground(data->deferredStage->getOSGStage()->getBackground());
        } else
#endif
        {
            data->win = PassiveWindow::create();
            data->view = Viewport::create();
            data->view->setRoot(getNode()->node);
            data->view->setBackground(data->stage->getBackground());
        }

        data->win->addPort(data->view);
        data->view->setSize(0, 0, 1, 1);
        data->view->setCamera(cam->getCam()->cam);
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

vector<VRTexturePtr> VRTextureRenderer::createCubeMaps(VRTransformPtr focusObject) {
    if (!cam) return {};
    if (!focusObject) return {};

    auto pose = cam->getPose();
    auto Near = cam->getNear();
    auto aspect = cam->getAspect();
    auto fov = cam->getFov();

    auto bb = focusObject->getBoundingbox();
    auto p = bb->center();
    auto s2 = bb->size()*0.51;

    cam->setAspect(1);
    cam->setFov(1.57079632679); // 90°

    cam->setNear(s2[2]);
    cam->setTransform(p, Vec3d(0,0,-1), Vec3d(0,-1,0));
    auto texFront = renderOnce();
    cam->setTransform(p, Vec3d(0,0,1), Vec3d(0,-1,0));
    auto texBack = renderOnce();

    cam->setNear(s2[0]);
    cam->setTransform(p, Vec3d(-1,0,0), Vec3d(0,-1,0));
    auto texLeft = renderOnce();
    cam->setTransform(p, Vec3d(1,0,0), Vec3d(0,-1,0));
    auto texRight = renderOnce();

    cam->setNear(s2[1]);
    cam->setTransform(p, Vec3d(0,1,0), Vec3d(0,0,1));
    auto texUp = renderOnce();
    cam->setTransform(p, Vec3d(0,-1,0), Vec3d(0,0,-1));
    auto texDown = renderOnce();

    cam->setAspect(aspect);
    cam->setFov(fov);
    cam->setNear(Near);
    cam->setPose(pose);
    return {texFront, texBack, texLeft, texRight, texUp, texDown};
}
