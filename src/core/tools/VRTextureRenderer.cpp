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
#include "core/setup/windows/VRView.h"
#include "core/scene/VRScene.h"
#include "core/scene/rendering/VRDefShading.h"
#include "core/math/partitioning/boundingbox.h"
#ifndef __EMSCRIPTEN__
#include "core/networking/tcp/VRTCPServer.h"
#endif

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
#include <OpenSG/OSGPassiveViewport.h>
#include <OpenSG/OSGFBOViewport.h>
#include <OpenSG/OSGRenderAction.h>
#include <OpenSG/OSGSolidBackground.h>
#include <OpenSG/OSGTreeBuilderBase.h>

#include "core/utils/VRMutex.h"

#define GLSL(shader) #shader

#define CHECK_GL_ERROR(msg) \
{ \
    GLenum err = glGetError(); \
    if (err != GL_NO_ERROR) { \
        static int i=0; i++; \
        if (i <= 9) printf(" gl error on %s: %s\n", msg, gluErrorString(err)); \
        if (i == 9) printf("  ..ignoring further errors\n"); \
    } \
}

using namespace OSG;



template<> string typeName(const VRTextureRenderer::CHANNEL* o) { return "VRTextureRenderer::CHANNEL"; }

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
    TextureBufferRefPtr     texBuf;
    TextureBufferRefPtr     texDBuf;
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

    data->texBuf = TextureBuffer::create();
    data->texBuf->setTexture(data->fboTex);

    data->fboDTexImg = Image::create();
    data->fboDTexImg->set(Image::OSG_RGB_PF, data->fboWidth, data->fboHeight/*, 1, 0, 1, 0, 0, Image::OSG_FLOAT32_IMAGEDATA*/);
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
    data->texDBuf = TextureBuffer::create();
    data->texDBuf->setTexture(data->fboDTex);

    RenderBufferRefPtr depthBuf = RenderBuffer::create();
    depthBuf->setInternalFormat(GL_DEPTH_COMPONENT24);

    data->fbo = FrameBufferObject::create();
    data->fbo->setColorAttachment(data->texBuf, 0);
    data->fbo->setDepthAttachment(data->texDBuf);
    data->fbo->editMFDrawBuffers()->push_back(GL_DEPTH_ATTACHMENT_EXT);
    data->fbo->editMFDrawBuffers()->push_back(GL_COLOR_ATTACHMENT0_EXT);
    data->fbo->setWidth (data->fboWidth );
    data->fbo->setHeight(data->fboHeight);
    data->fbo->setPostProcessOnDeactivate(true);

    if (readback) {
        data->texBuf->setReadBack(true);
        data->texDBuf->setReadBack(true);
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

VRMaterialPtr VRTextureRenderer::copyMaterial() {
    if (!data) return 0;
    auto mat = VRMaterial::create("VRTextureRenderer");
    mat->setTexture(data->fboTex, 0);
    mat->setTexture(data->fboDTex, 1);
    return mat;
}

VRTextureRenderer::~VRTextureRenderer() { stopServer(); delete data; }

VRTextureRendererPtr VRTextureRenderer::create(string name, bool readback) {
    auto tg = VRTextureRendererPtr( new VRTextureRenderer(name, readback) );
    TRinstances.push_back(tg);
    return tg;
}

Vec2i VRTextureRenderer::getResolution() {
    return Vec2i(data->fboWidth, data->fboHeight);
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
        data->fboDTexImg->set(Image::OSG_RGBA_PF, data->fboWidth, data->fboHeight/*, 1, 0, 1, 0, 0, Image::OSG_FLOAT32_IMAGEDATA*/);
    }

    else {
        data->fboTexImg->set(Image::OSG_RGB_PF, data->fboWidth, data->fboHeight);
        data->fboDTexImg->set(Image::OSG_RGB_PF, data->fboWidth, data->fboHeight/*, 1, 0, 1, 0, 0, Image::OSG_FLOAT32_IMAGEDATA*/);
    }
    data->stage->setCamera(cam->getCam()->cam);
#ifndef WITHOUT_DEFERRED_RENDERING
    if (data->deferredStage) data->deferredStage->setDSCamera( cam->getCam() );
#endif
}

void VRTextureRenderer::setReadback(bool RGBReadback, bool depthReadback) {
    data->texBuf->setReadBack(RGBReadback);
    data->texDBuf->setReadBack(depthReadback);
}

bool _convertToUInt8(ImageMTRecPtr img) {
    auto dtype = img->getDataType();
    auto format = img->getPixelFormat();
    Vec3i s = Vec3i(img->getWidth(), img->getHeight(), img->getDepth());

    if (dtype == OSG::Image::OSG_FLOAT32_IMAGEDATA && format == OSG::Image::OSG_RGB_PF) {
        size_t N = s[0]*s[1]*s[2];
        Color3f* src = (Color3f*)img->getData();
        vector<Color3ub> dst(N);
        for (size_t i=0; i<N; i++) {
            Color3f c = src[i];
            UInt8 r = max(0.f,c[0])*255;
            UInt8 g = max(0.f,c[1])*255;
            UInt8 b = max(0.f,c[2])*255;
            dst[i] = Color3ub(r,g,b);
        }
        img->set(format, s[0], s[1], s[2], 1, 1, 0, (UInt8*)&dst[0]);
        return true;
    }
    return false;
}

// This doesnt work because OpenSG doesnt bind GL_DEPTH_ATTACHMENT
// I changed that but this would need some testing!
void VRTextureRenderer::exportDepthImage(string path) {
    //auto tBuf = data->texBuf;
    auto tBuf = data->texDBuf;

    bool b = tBuf->getReadBack();
    if (!b) cout << "Warning! depth buffer readback disabled! image export will fail!" << endl;

    auto fboTex = tBuf->getTexture();
    auto img = fboTex->getImage();
    auto dtype = img->getDataType();
    auto format = img->getPixelFormat();

    cout << "VRTextureRenderer::exportDepthImage to " << path << endl;
    cout << " img type: " << VRTexture::typeToString(dtype) << endl;
    cout << " img format: " << VRTexture::formatToString(format) << endl;

    int w = img->getWidth();
    int h = img->getHeight();
    int d = img->getDepth();
    int s = w*h*d;
    size_t bS = img->getSize(0,0,0);
    cout << " dimensions: " << Vec3i(w,h,d) << " -> N pixels: " << s << ", byte size: " << bS << endl;

    double T = 0;
    for (size_t i=0; i<bS; i++) T += (int)img->getData()[i];
    cout << " byteSum: " << T << endl;
    if (dtype != OSG::Image::OSG_UINT8_IMAGEDATA) { // need to convert image data to write to file
        cout << "convert image data from " << dtype << " to OSG::Image::OSG_UINT8_IMAGEDATA" << endl;
        ImageMTRecPtr img2 = Image::create();
        img2->set(img);
        if (!_convertToUInt8(img2)) img2->convertDataTypeTo(OSG::Image::OSG_UINT8_IMAGEDATA);
        img = img2;
    }

    if (format == OSG::Image::OSG_A_PF) { // need to convert pixel data
        cout << "convert pixel data from " << format << " to OSG::Image::OSG_RGB_PF" << endl;
        ImageMTRecPtr img2 = Image::create();
        img->reformat(OSG::Image::OSG_RGB_PF, img2);
        img = img2;
    }

    if (!img->write(path.c_str())) cout << " VRTexture::write to file '" << path << "' failed!" << endl;
}

void VRTextureRenderer::setCam(VRCameraPtr c) { cam = c; }
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

class MyFrameBufferObject : public FrameBufferObject {
    public:

    void myactivate(DrawEnv *pEnv, GLenum   eDrawBuffer) {
        Window *win = pEnv->getWindow();
        UInt32 glId = getGLId();
        win->validateGLObject(getGLId(), pEnv);

        if(_sfEnableMultiSample.getValue()                      == true &&
            win->hasExtOrVersion(_uiFramebufferBlitExt, 0x0300) == true   ) {
                win->validateGLObject(getMultiSampleGLId(), pEnv);
                glId = getMultiSampleGLId();
                glEnable(GL_MULTISAMPLE);
        }

        OSGGETGLFUNCBYID_GL3_ES( glBindFramebuffer, osgGlBindFramebuffer, _uiFuncBindFramebuffer, win);
        osgGlBindFramebuffer(GL_FRAMEBUFFER, win->getGLObjectId(glId));
        pEnv->setActiveFBO(glId);
    }
};

#ifndef __EMSCRIPTEN__
class OpenRenderPartition : public RenderPartition {
    public:
        void setupMyExecution() {
            auto target = (MyFrameBufferObject*)_pRenderTarget;
            target->myactivate(&_oDrawEnv, _eDrawBuffer);

            glPushAttrib(GL_VIEWPORT_BIT | GL_SCISSOR_BIT);
            glViewport(_oDrawEnv.getPixelLeft  (), _oDrawEnv.getPixelBottom(), _oDrawEnv.getPixelWidth (), _oDrawEnv.getPixelHeight());

            glMatrixMode (GL_PROJECTION);
            glPushMatrix();
            glLoadMatrixf(_oDrawEnv._openGLState.getProjection().getValues());
            glMatrixMode(GL_MODELVIEW);

            _pBackground->clear(&_oDrawEnv);
        }

        void doMyExecution(bool bRestoreViewport = false) {
            BuildKeyMapIt      mapIt  = _mMatTrees.begin();
            BuildKeyMapConstIt mapEnd = _mMatTrees.end  ();
            while (mapIt != mapEnd) {
                if (mapIt->second != NULL) mapIt->second->draw(_oDrawEnv, this);
                ++mapIt;
            }

            _oDrawEnv.deactivateState();
            if (!_bZWriteTrans) glDepthMask(false);

            mapIt  = _mTransMatTrees.begin();
            mapEnd = _mTransMatTrees.end  ();
            while (mapIt != mapEnd) {
                if (mapIt->second != NULL) mapIt->second->draw(_oDrawEnv, this);
                ++mapIt;
            }

            if (!_bZWriteTrans) glDepthMask(true);

            glMatrixMode (GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);

            glPopAttrib();
            _pRenderTarget->deactivate(&_oDrawEnv);
            this->exit();
        }

        void myexecute(void) {
            setupMyExecution();
            doMyExecution();
        }
};

class OpenRenderAction : public RenderAction {
    public:
        Action::FunctorStore* getEnterFunctors() { return getDefaultEnterFunctors(); }
        Action::FunctorStore* getLeaveFunctors() { return getDefaultEnterFunctors(); }

        //Action::ResultE traverse(Node* const node) { return recurse(node); }

        Action::ResultE callenter(NodeCore* const core) {
            UInt32 uiFunctorIndex = core->getType().getId();
            return _enterFunctors[uiFunctorIndex](core, this);
        }

        Action::ResultE traverse(Node* const node) {
            NodeCore* core = node->getCore();

            Action::ResultE result = Continue;

            _actList   = NULL;
            _actNode   = node;
            _actParent = node;

            _newList.clear();
            _useNewList = false;

            result = callenter(node->getCore());

            _actNode   = node;
            _actParent = node;

            Node::MFChildrenType::const_iterator cIt = node->getMFChildren()->begin();
            Node::MFChildrenType::const_iterator cEnd = node->getMFChildren()->end  ();

            for(; cIt != cEnd; ++cIt) {
                result = recurse(*cIt);
                if (result != Continue) break;
            }

            _actNode   = node;
            _actParent = node;
            result = callLeave(node->getCore());

            _actNode   = node;
            _actParent = node;
            return Skip;
        }

        Action::ResultE mystop(ResultE res) {
            Inherited::stop(res);
            auto buf = _currentBuffer;
            for (PtrDiffT i = _vRenderPartitions[buf].size() - 1; i > 0; --i) {
                auto partI = (OpenRenderPartition*)_vRenderPartitions[buf][i];
                partI->myexecute();
            }
            return Action::Continue;
        }
};
#endif

VRTexturePtr VRTextureRenderer::renderOnce(CHANNEL c) { // TODO: not working!
    if (!cam) return 0;
    bool deferred = VRScene::getCurrent()->getDefferedShading();

    if (!data->ract) {
        data->ract = RenderAction::create();
        data->ract->setFrustumCulling(false);
        data->ract->setOcclusionCulling(false);

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
            data->view = Viewport::create(); // PassiveViewport or FBOViewport ?

            data->view->setRoot(getNode()->node);
            data->view->setBackground(data->stage->getBackground());
        }

        data->win->addPort(data->view);
        data->view->setSize(0.5, 0.5, 0.5, 0.5);
        data->view->setCamera(cam->getCam()->cam);
    }

    if (c != RENDER) setChannelSubstitutes(c);

    setActive(true);
    setReadback(true, true);

#ifdef __EMSCRIPTEN__
    data->win->render(data->ract);
#else
    data->win->frameInit();
    data->ract->setWindow(data->win);
    data->ract->setTraversalRoot(getNode()->node);
    data->ract->setTravMask     (data->view->getTravMask()  );
    data->ract->setRenderProperties(0);

    auto ract = (OpenRenderAction*)data->ract.get();
    auto core = getNode()->node->getCore();
    UInt32 uiFunctorIndex = core->getType().getId();

    //data->ract->apply( getNode()->node );
    ract->start();
    auto res = ract->traverse(getNode()->node);
    ract->mystop(res);
    CHECK_GL_ERROR("renderOnce");
#endif

    //data->win->render(data->ract);
    //data->win->renderNoFinish(data->ract);
    if (deferred) data->win->render(data->ract); // hack, TODO: for some reason the fbo gets not updated the first render call..
    setReadback(false, false);
    setActive(false);

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

void VRTextureRenderer::prepareTextureForStream() {
    VRLock lock(mtx);
    mat->getTexture()->write("tmp2.jpeg");
}

string VRTextureRenderer::startServer(int port) {
    string uri;
#ifndef __EMSCRIPTEN__
    server = VRTCPServer::create();
    server->onMessage( bind(&VRTextureRenderer::serverCallback, this, _1) );
    server->listen(port, "\r\n\r\n");
    updateCb = VRUpdateCb::create("texture renderer stream", bind(&VRTextureRenderer::prepareTextureForStream, this));
    VRScene::getCurrent()->addUpdateFkt(updateCb);
#endif
    return uri;
}

void VRTextureRenderer::stopServer() {
#ifndef __EMSCRIPTEN__
    if (!server) return;
    VRScene::getCurrent()->dropUpdateFkt(updateCb);
    server->close();
    server.reset();
#endif
}

const string restImgHeader =
"HTTP/1.1 200 OK\n"
"Date: Thu, 19 Feb 2009 12:27:04 GMT\n"
"Server: Apache/2.2.3\n"
"Last-Modified: Fri, 10 Feb 2012 14:31:06 GMT\n"
"ETag: \"56d-9989200-1132c580\"\n"
"Content-Type: image/jpeg\n"
"Connection: close\n"
"Content-Length: ";

string VRTextureRenderer::serverCallback(string data) {
    VRLock lock(mtx);
    //cout << "VRTextureRenderer::serverCallback, received: " << data << endl;

    ifstream fin("tmp2.jpeg", ios::binary);
    ostringstream ostrm;
    ostrm << fin.rdbuf();

    string imgData = ostrm.str();
    size_t N = imgData.size();
    return restImgHeader + toString(N) + "\n\n" + imgData;
}
