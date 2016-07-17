#include "VRView.h"
#include <OpenSG/OSGRenderAction.h>
#include <OpenSG/OSGImageForeground.h>
#include <libxml++/nodes/element.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGMultiPassMaterial.h>

#include "core/utils/VRRate.h"
#include "core/utils/toString.h"
#include "core/tools/VRText.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/gui/VRGuiUtils.h"
#include "core/gui/VRGuiManager.h"
#include "core/objects/OSGObject.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/geometry/VRSprite.h"
#include "core/objects/VRTransform.h"
#include "core/objects/VRCamera.h"
#include "core/objects/object/VRObjectT.h"

#include "core/objects/VRLight.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRDefShading.h"
#include "core/scene/VRSSAO.h"
#include "core/scene/VRHMDDistortion.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

bool onBox(int i, int j, int c) {
    if(abs(i) > c || abs(j) > c) return false;
    if(abs(i) == c || abs(j) == c) return true;
    return false;
}

string VRView::getName() { return name; }

void VRView::setMaterial() {
    ImageRecPtr img = Image::create();

    Vec4f bg = Vec4f(0.5, 0.7, 0.95, 0.5);
    Vec4f c1 = Vec4f(0.5, 0.7, 0.95, 0.9);
    Vec4f cax = Vec4f(0.9, 0.2, 0.2, 1);
    Vec4f cay = Vec4f(0.2, 0.9, 0.2, 1);

    auto label = VRText::get()->create(name, "SANS 20", 20, Color4f(0,0,0,255), Color4f(bg[2]*255.0, bg[1]*255.0, bg[0]*255.0, 0));
    float lw = label->getImage()->getWidth();
    float lh = label->getImage()->getHeight();

    int s=256;
    int b1 = 0.5*s-8;
    int b2 = 0.5*s-50;
    int ar = 50;
    Vec2f pl(-0.1*s, -0.05*s);

	vector<Vec4f> data;
	data.resize(s*s);

    for (int i=0; i<s; i++) {
        for (int j=0; j<s; j++) {
            int k = i+j*s;
            data[k] = bg;

            int x = i-0.5*s;
            int y = j-0.5*s;

            if (onBox(x,y,b1)) data[k] = c1; // box1
            if (onBox(x,y,b2)) data[k] = c1; // box2

            if (y == 0 && x >= 0 && x < ar) data[k] = cax; // ax
            if (x == 0 && y >= 0 && y < ar) data[k] = cay; // ax

            if (x >= pl[0]-lw*0.5 && x < pl[0]+lw*0.5 && y >= pl[1]-lh*0.5 && y < pl[1]+lh*0.5) {
                int u = x - pl[0] + lw*0.5;
                int v = y - pl[1] + lh*0.5;
                int w = 4*(u+v*lw);
                const UInt8* d = label->getImage()->getData();
                data[k] = Vec4f(d[w]/255.0, d[w+1]/255.0, d[w+2]/255.0, d[w+3]/255.0);
                //data[k] = Vec4f(1,1,1, 1);
            }
        }
    }

    img->set( Image::OSG_RGBA_PF, s, s, 1, 0, 1, 0, (const uint8_t*)&data[0], OSG::Image::OSG_FLOAT32_IMAGEDATA, true, 1);

    viewGeoMat->setTexture(VRTexture::create(img));
    viewGeoMat->setLit(false);
}

void VRView::setViewports() {//create && set size of viewports
    if (window && lView) window->subPortByObj(lView);
    if (window && rView) window->subPortByObj(rView);
    lView = 0;
    rView = 0;

    Vec4f p = position;
    p[1] = 1-position[3]; // invert y
    p[3] = 1-position[1];

    if (p[0] > p[2]) p[0] = p[2]-0.01;
    if (p[1] > p[3]) p[1] = p[3]-0.01;


    //active, stereo
    if (active_stereo) {
        lView_act = StereoBufferViewport::create();
        rView_act = StereoBufferViewport::create();
    } else {
        lView_act = 0;
        rView_act = 0;
    }

    //no stereo
    if (!stereo && !active_stereo) {
        lView = Viewport::create();
        lView->setSize(p[0], p[1], p[2], p[3]);
        rView = 0;
        return;
    }

    if (stereo && !active_stereo) {
        lView = Viewport::create();
        rView = Viewport::create();
        lView->setSize(p[0], p[1], (p[0]+p[2])*0.5, p[3]);
        rView->setSize((p[0]+p[2])*0.5, p[1], p[2], p[3]);
        return;
    }


    lView_act->setLeftBuffer(true);
    lView_act->setRightBuffer(false);
    rView_act->setLeftBuffer(false);
    rView_act->setRightBuffer(true);

    lView = lView_act;
    rView = rView_act;

    lView->setSize(p[0], p[1], p[2], p[3]);
    rView->setSize(p[0], p[1], p[2], p[3]);
}

void VRView::setBG() {
    if (background) {
        if (lView) lView->setBackground(background);
        if (rView) rView->setBackground(background);
    }
}

void VRView::setDecorators() {//set decorators, only if projection true
    if (projection) { // put in setProjection fkt

        //View
        proj_normal.normalize();
        proj_up.normalize();
        float w = proj_size[0];
        float h = proj_size[1];
        Vec3f x = proj_normal.cross(proj_up);

        screenLowerLeft = Pnt3f(proj_center - proj_up*h*(0.5+proj_warp[1]+proj_shear[1]) + x*w*(0.5-proj_warp[0]+proj_shear[0]));
        screenLowerRight = Pnt3f(proj_center - proj_up*h*(0.5-proj_warp[1]-proj_shear[1]) - x*w*(0.5-proj_warp[0]-proj_shear[0]));
        screenUpperRight = Pnt3f(proj_center + proj_up*h*(0.5-proj_warp[1]+proj_shear[1]) - x*w*(0.5+proj_warp[0]+proj_shear[0]));
        screenUpperLeft = Pnt3f(proj_center + proj_up*h*(0.5+proj_warp[1]-proj_shear[1]) + x*w*(0.5+proj_warp[0]-proj_shear[0]));
    } else {
        screenLowerLeft = Pnt3f(-1,-0.6, -1);
        screenLowerRight = Pnt3f(1,-0.6, -1);
        screenUpperRight = Pnt3f(1,0.6, -1);
        screenUpperLeft = Pnt3f(-1,0.6, -1);
    }

    //cout << "setDecorator screen: shear " << proj_shear << " warp " << proj_warp << endl;
    //cout << "setDecorator screen: LL " << screenLowerLeft << " LR " << screenLowerRight << " UR " << screenUpperRight << " UL " << screenUpperLeft << " " << endl;

    GeometryMTRecPtr geo = dynamic_cast<Geometry*>(viewGeo->getCore());
    GeoVectorPropertyRecPtr pos = geo->getPositions();

    pos->setValue(screenLowerLeft, 0);
    pos->setValue(screenLowerRight, 1);
    pos->setValue(screenUpperLeft, 2);
    pos->setValue(screenUpperRight, 3);

    if (!projection && !stereo) {
        PCDecoratorLeft = 0;
        PCDecoratorRight = 0;
        return;
    }

    if (projection && !stereo) {
        cout << "\nset single projection decorator";
        PCDecoratorLeft = ProjectionCameraDecorator::create();
        PCDecoratorLeft->setLeftEye(true);
        PCDecoratorLeft->setEyeSeparation(0);
        PCDecoratorLeft->editMFSurface()->clear();
        PCDecoratorLeft->editMFSurface()->push_back(screenLowerLeft);
        PCDecoratorLeft->editMFSurface()->push_back(screenLowerRight);
        PCDecoratorLeft->editMFSurface()->push_back(screenUpperRight);
        PCDecoratorLeft->editMFSurface()->push_back(screenUpperLeft);
        PCDecoratorRight = 0;
        return;
    }

    // stereo

    cout << "\nset projection decorators";
    PCDecoratorLeft = ProjectionCameraDecorator::create();
    PCDecoratorRight = ProjectionCameraDecorator::create();

    PCDecoratorLeft->setLeftEye(false);
    PCDecoratorLeft->setEyeSeparation(0.06);
    PCDecoratorLeft->editMFSurface()->clear();
    PCDecoratorLeft->editMFSurface()->push_back(screenLowerLeft);
    PCDecoratorLeft->editMFSurface()->push_back(screenLowerRight);
    PCDecoratorLeft->editMFSurface()->push_back(screenUpperRight);
    PCDecoratorLeft->editMFSurface()->push_back(screenUpperLeft);

    PCDecoratorRight->setLeftEye(true);
    PCDecoratorRight->setEyeSeparation(0.06);
    PCDecoratorRight->editMFSurface()->clear();
    PCDecoratorRight->editMFSurface()->push_back(screenLowerLeft);
    PCDecoratorRight->editMFSurface()->push_back(screenLowerRight);
    PCDecoratorRight->editMFSurface()->push_back(screenUpperRight);
    PCDecoratorRight->editMFSurface()->push_back(screenUpperLeft);
}

VRView::VRView(string name) {
    this->name = name;

    SolidBackgroundRecPtr sbg = SolidBackground::create();
    sbg->setColor(Color3f(0.7, 0.7, 0.7));
    background = sbg;

    dummy_user = VRTransform::create("view_user");
    dummy_user->setPersistency(0);

    viewGeo = makeNodeFor(makePlaneGeo(1,1,1,1));
    viewGeo->setTravMask(0);
    viewGeoMat = VRMaterial::create("setup view mat");
    GeometryMTRecPtr geo = dynamic_cast<Geometry*>( viewGeo->getCore() );
    geo->setMaterial(viewGeoMat->getMaterial());


    root_system = VRObject::create("System root");
    root_post_processing = VRObject::create("Post processing root");
    root_def_shading = VRObject::create("Deffered shading root");

    root_system->addChild(root_post_processing);
    root_post_processing->addChild(root_def_shading);

    /*auto ssao_mat = setupRenderLayer("ssao", root_def_shading);
    auto calib_mat = setupRenderLayer("calibration", root_post_processing);
    auto hmdd_mat = setupRenderLayer("hmdd", root_post_processing);
    //auto metaball_mat = setupRenderLayer("metaball");

    defShading = new VRDefShading();
    ssao = new VRSSAO();
    hmdd = new VRHMDDistortion();
    auto hmddPtr = shared_ptr<VRHMDDistortion>(hmdd);

    root_post_processing->addChild( hmddPtr );
    root_def_shading->switchParent( hmddPtr );

    defShading->initDeferredShading(root_def_shading);
    ssao->initSSAO(ssao_mat);
    hmdd->initHMDD(hmdd_mat);
    hmdd_mat->setTexture(defShading->getTarget(), 0);
    initCalib(calib_mat);
    setDefferedShading(false);
    setSSAO(false);
    setHMDD(false);*/


    update();
    update2();
}

VRView::~VRView() {
    window->subPortByObj(lView);
    window->subPortByObj(rView);
    window->subPortByObj(lView_act);
    window->subPortByObj(rView_act);
    lView = 0;
    rView = 0;
    lView_act = 0;
    rView_act = 0;
    PCDecoratorLeft = 0;
    PCDecoratorRight = 0;
    stats = 0;
}

VRViewPtr VRView::create(string name) { return VRViewPtr(new VRView(name)); }
VRViewPtr VRView::ptr() { return shared_from_this(); }

int VRView::getID() { return ID; }
void VRView::setID(int i) { ID = i; }

VRMaterialPtr VRView::setupRenderLayer(string name, VRObjectPtr parent) {
    auto plane = VRGeometry::create(name+"_renderlayer");
    plane->setPrimitive("Plane", "2 2 1 1");
    plane->setVolume(false);
    plane->setMaterial( VRMaterial::create(name+"_mat") );
    parent->addChild(plane);
    renderLayer[name] = plane;
    return plane->getMaterial();
}

void VRView::initCalib(VRMaterialPtr mat) {
    string shdrDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/DeferredShading/";
    mat->setLit(false);
    mat->readVertexShader(shdrDir + "Calib.vp.glsl");
    mat->readFragmentShader(shdrDir + "Calib.fp.glsl");
    mat->setShaderParameter<int>("grid", 64);
}

void VRView::showStats(bool b) {
    if (stats == 0) {
        stats = SimpleStatisticsForeground::create();

        stats->setSize(25);
        stats->setColor(Color4f(0,1,0,0.7f));

        // Render traversal stats action
        stats->addElement(RenderAction::statDrawTime, "Draw FPS: %r.3f");
        stats->addElement(RenderAction::statTravTime, "Trav FPS: %r.3f");
        stats->addElement(VRRate::statFPStime, "PVR FPS: %d");
        stats->addElement(VRRate::statPhysFPStime, "Physics FPS: %d");

        stats->addElement(RenderAction::statNStates, " State changes: %d");
        stats->addElement(RenderAction::statNShaders, "Shader changes: %d");
        stats->addElement(RenderAction::statNShaderParams, "Shader param changes: %d");
        stats->addElement(TextureObjChunk::statNTextures, "Textures: %d");
        stats->addElement(TextureObjChunk::statNTexBytes, " Tex Mem: %MB MB");


        stats->addElement(RenderAction::statNMatrices, "Matrix changes: %d");
        //stats->addElement(RenderAction::statNGeometries, "    Geom nodes: %d");
        //stats->addElement(RenderAction::statNTransGeometries, "Transparent Nodes drawn   %d");
        //stats->addElement(RenderAction::statNTriangles, "     Triangles: %d");
        //stats->addElement(RenderAction::statNMaterials, "%d material changes");

        //stats->addElement(PointLight::statNPointLights, "%d active point lights");
        //stats->addElement(DirectionalLight::statNDirectionalLights, "%d active directional lights");
        //stats->addElement(SpotLight::statNSpotLights, "%d active spot lights");

        stats->addText   ("Drawables: (drawn)");
        stats->addElement(Drawable::statNTriangles,    "  tris: %d");
        stats->addElement(Drawable::statNLines,        " lines: %d");
        stats->addElement(Drawable::statNPoints,       "points: %d");
        stats->addElement(Drawable::statNVertices,     " verts: %d");

        if(stats->getCollector() != NULL) stats->getCollector()->getElem(Drawable::statNTriangles);

        stats->addText   ("ChangeList: ");
        stats->addElement(ChangeList::statNChangedStoreSize, "    %d entries in changedStore");
        stats->addElement(ChangeList::statNCreatedStoreSize, "    %d entries in createdStore");
        stats->addElement(ChangeList::statNUnCommittedStoreSize, "    %d entries in uncommitedStore");
        stats->addElement(ChangeList::statNPoolSize, "    %d entries in pool");


        //stats->addElement(DrawActionBase::statCullTestedNodes, "Nodes culltested      %d");
        //stats->addElement(DrawActionBase::statCulledNodes, "Nodes culled          %d");
        //stats->addElement(RenderAction::statNOcclusionMode, "Occlusion culling     %s");
        //stats->addElement(RenderAction::statNOcclusionTests, "Occlusion tests       %d");
        //stats->addElement(RenderAction::statNOcclusionCulled, "Occlusion culled      %d");


        if(stats->getCollector() != NULL) stats->editCollector()->getElem(Drawable::statNTriangles);
    }

    if (lView == 0) return;
    if (b && !doStats) lView->addForeground(stats);
    if (!b && doStats) lView->removeObjFromForegrounds(stats);
    doStats = b;

    VRSetupManager::getCurrent()->getRenderAction()->setStatCollector(stats->getCollector());
}

void VRView::showViewGeo(bool b) {
    if (b) viewGeo->setTravMask(0xffffffff);
    else viewGeo->setTravMask(0);
}

Vec4f VRView::getPosition() { return position; }
void VRView::setPosition(Vec4f pos) { position = pos; update(); }

void VRView::setRoot(VRObjectPtr root, VRTransformPtr real) { view_root = root; real_root = real; update(); }

void VRView::setRoot() {
    if (real_root && viewGeo) real_root->addChild(OSGObject::create(viewGeo));

    if (user && real_root) user->switchParent(real_root);
    if (dummy_user && real_root) dummy_user->switchParent(real_root);

    if (view_root) {
        root_def_shading->addLink( view_root );
    }

    NodeMTRecPtr n = root_system ? root_system->getNode()->node : 0;
    if (lView) lView->setRoot(n);
    if (rView) rView->setRoot(n);
}

void VRView::setUser(VRTransformPtr u) {
    user = u;
    user_name = user ? user->getName() : "";
    update();
}

void VRView::setUser() {
    if (user == 0 && user_name != "") user = VRSetupManager::getCurrent()->getTracker(user_name);

    if (user == 0) {
        if (PCDecoratorLeft) PCDecoratorLeft->setUser(dummy_user->getNode()->node);
        if (PCDecoratorRight) PCDecoratorRight->setUser(dummy_user->getNode()->node);
    } else {
        user_name = user->getName();
        if (PCDecoratorLeft) PCDecoratorLeft->setUser(user->getNode()->node);
        if (PCDecoratorRight) PCDecoratorRight->setUser(user->getNode()->node);
    }
}

void VRView::setCamera(VRCameraPtr c) { cam = c; update(); }

void VRView::setCam() {
    if (cam == 0) return;

    if (lView && PCDecoratorLeft == 0) lView->setCamera(cam->getCam());
    if (rView && PCDecoratorRight == 0) rView->setCamera(cam->getCam());

    if (PCDecoratorLeft) PCDecoratorLeft->setDecoratee(cam->getCam());
    if (PCDecoratorRight) PCDecoratorRight->setDecoratee(cam->getCam());

    if (lView && PCDecoratorLeft) lView->setCamera(PCDecoratorLeft);
    if (rView && PCDecoratorRight) rView->setCamera(PCDecoratorRight);

    if (defShading) defShading->setDSCamera(cam); // TODO: pass decorators!
    if (hmdd) hmdd->setCamera(cam);
}

void VRView::setBackground(BackgroundRecPtr bg) { background = bg; update(); }

void VRView::setWindow(WindowRecPtr win) { window = win; update(); }

void VRView::setWindow() {
    if (window == 0) return;
    if (lView) window->addPort(lView);
    if (rView) window->addPort(rView);
}

void VRView::setStereo(bool b) { stereo = b; update(); }
void VRView::setActiveStereo(bool b) { active_stereo = b; update(); }

void VRView::setStereoEyeSeparation(float v) {
    eyeSeparation = v;
    if (PCDecoratorLeft) PCDecoratorLeft->setEyeSeparation(v);
    if (PCDecoratorRight) PCDecoratorRight->setEyeSeparation(v);
}

void VRView::swapEyes(bool b) {
    eyeinverted = b;
    if (PCDecoratorLeft) PCDecoratorLeft->setLeftEye(!b);
    if (PCDecoratorRight) PCDecoratorRight->setLeftEye(b);
}

bool VRView::eyesInverted() { return eyeinverted; }
bool VRView::activeStereo() { return active_stereo; }

void VRView::update() {
    setViewports();
    setDecorators();
    setCam();
    setRoot();
    setUser();
    setWindow();
    setBG();
    swapEyes(eyeinverted);
    setStereoEyeSeparation(eyeSeparation);
    setMaterial();
}

void VRView::update2() {
    if (defShading) defShading->setDefferedShading(deferredRendering);
    if (ssao) ssao->setSSAOparams(ssao_radius, ssao_kernel, ssao_noise);

    for (auto m : VRMaterial::materials) {
        auto mat = m.second.lock();
        if (!mat) continue;
        bool b = ( (ssao && do_ssao) || (defShading && deferredRendering) );
        mat->setDeffered(b);
    }

    // update shader code
    if (defShading) defShading->reload();
    if (do_hmdd && hmdd) hmdd->reload();
    if (hmdd) hmdd->setActive(do_hmdd);

    // update render layer visibility
    if (renderLayer.count("ssao")) renderLayer["ssao"]->setVisible(do_ssao);
    if (renderLayer.count("calibration")) renderLayer["calibration"]->setVisible(calib);
    if (renderLayer.count("hmdd")) renderLayer["hmdd"]->setVisible(do_hmdd);
}

void VRView::addLight(VRLightPtr l) {
    light_map[l->getID()] = l;
    if (defShading) defShading->addDSLight(l);
}

void VRView::setDefferedShading(bool b) { deferredRendering = b; update2(); }
void VRView::setSSAO(bool b) { do_ssao = b; update2(); }
void VRView::setSSAOradius(float r) { ssao_radius = r; update2(); }
void VRView::setSSAOkernel(int k) { ssao_kernel = k; update2(); }
void VRView::setSSAOnoise(int k) { ssao_noise = k; update2(); }
void VRView::setCalib(bool b) { calib = b; update2(); }
void VRView::setHMDD(bool b) { do_hmdd = b; update2(); }

void VRView::reset() {
    cam = 0;
    view_root = 0;
    update();
}

void VRView::setFotoMode(bool b) {
    if (!stereo) return;
    if (b) {
        /*SolidBackgroundRecPtr sbg = SolidBackground::create();
        sbg->setColor(Color3f(0,0,0));
        if (rView) rView->setBackground(sbg);
        NodeMTRecPtr n = Node::create();
        if (rView) rView->setRoot(n);*/
        if (PCDecoratorLeft) PCDecoratorLeft->setEyeSeparation(0);
        if (PCDecoratorRight) PCDecoratorRight->setEyeSeparation(0);
    } else update();
}

VRTexturePtr VRView::grab() {
    return takeSnapshot();

    /*if (grabfg == 0) {
        grabfg = GrabForeground::create();
        ImageRecPtr img = Image::create();
        grabfg->setImage(img);
        grabfg->setActive(false);
        if (lView) lView->editMFForegrounds()->push_back(grabfg);
    }


    if (lView) {
        grabfg->setActive(true);
        OSG::commitChanges();

        //window->render( VRSetupManager::getCurrent()->getRenderAction() );
        VRSetupManager::getCurrent()->updateWindows();
        VRGuiManager::get()->updateGtk(); // TODO: Rendering produces just opengl error 500

        img = grabfg->getImage();
        if (img->getData()) img->write("bla.png");
        cout << "GRAB " << img->getData() << endl;
        grabfg->setActive(false);
    }
    return img;*/
}

void VRView::save(xmlpp::Element* node) {
    node->set_attribute("stereo", toString(stereo).c_str());
    node->set_attribute("active_stereo", toString(active_stereo).c_str());
    node->set_attribute("projection", toString(projection).c_str());
    node->set_attribute("eye_inverted", toString(eyeinverted).c_str());
    node->set_attribute("eye_separation", toString(eyeSeparation).c_str());
    node->set_attribute("position", toString(position).c_str());
    node->set_attribute("center", toString(proj_center).c_str());
    node->set_attribute("normal", toString(proj_normal).c_str());
    node->set_attribute("user_pos", toString(proj_user).c_str());
    node->set_attribute("up", toString(proj_up).c_str());
    node->set_attribute("size", toString(proj_size).c_str());
    node->set_attribute("shear", toString(proj_shear).c_str());
    node->set_attribute("warp", toString(proj_warp).c_str());
    if (user) node->set_attribute("user", user->getName());
    else node->set_attribute("user", user_name);
}

void VRView::load(xmlpp::Element* node) {
    stereo = toBool(node->get_attribute("stereo")->get_value());
    active_stereo = toBool(node->get_attribute("active_stereo")->get_value());
    projection = toBool(node->get_attribute("projection")->get_value());
    eyeinverted = toBool(node->get_attribute("eye_inverted")->get_value());
    eyeSeparation = toFloat(node->get_attribute("eye_separation")->get_value());
    position = toVec4f(node->get_attribute("position")->get_value());
    proj_center = toVec3f(node->get_attribute("center")->get_value());
    proj_normal = toVec3f(node->get_attribute("normal")->get_value());
    if (node->get_attribute("user_pos")) proj_user = toVec3f(node->get_attribute("user_pos")->get_value());
    proj_up = toVec3f(node->get_attribute("up")->get_value());
    proj_size = toVec2f(node->get_attribute("size")->get_value());
    if (node->get_attribute("shear")) proj_shear = toVec2f(node->get_attribute("shear")->get_value());
    if (node->get_attribute("warp")) proj_warp = toVec2f(node->get_attribute("warp")->get_value());
    if (node->get_attribute("user")) {
        user_name = node->get_attribute("user")->get_value();
        user = VRSetupManager::getCurrent()->getTracker(user_name);
    }

    dummy_user->setFrom(proj_user);
    showStats(doStats);
    update();
}

VRTransformPtr VRView::getUser() { if (user) return user; else return dummy_user; }
VRCameraPtr VRView::getCamera() { return cam; }
ViewportRecPtr VRView::getViewport() { return lView; }
float VRView::getEyeSeparation() { return eyeSeparation; }
bool VRView::isStereo() { return stereo; }

void VRView::setProjection(bool b) { projection = b; update(); }
bool VRView::isProjection() { return projection; }

void VRView::setProjectionUp(Vec3f v) { proj_up = v; update(); }
Vec3f VRView::getProjectionUp() { return proj_up; }
void VRView::setProjectionNormal(Vec3f v) { proj_normal = v; update(); }
Vec3f VRView::getProjectionNormal() { return proj_normal; }
void VRView::setProjectionCenter(Vec3f v) { proj_center = v; update(); }
Vec3f VRView::getProjectionCenter() { return proj_center; }
void VRView::setProjectionSize(Vec2f v) { proj_size = v; update(); }
Vec2f VRView::getProjectionSize() { return proj_size; }
void VRView::setProjectionShear(Vec2f v) { proj_shear = v; update(); }
Vec2f VRView::getProjectionShear() { return proj_shear; }
void VRView::setProjectionWarp(Vec2f v) { proj_warp = v; update(); }
Vec2f VRView::getProjectionWarp() { return proj_warp; }

void VRView::setProjectionUser(Vec3f v) {
    proj_user = v; update();

    cout << "VRView::setProjectionUser\n";
    /*setViewports();
    setDecorators();
    setCam();
    setRoot();*/
    //setUser();
    /*setWindow();
    setBG();
    swapEyes(eyeinverted);
    setStereoEyeSeparation(eyeSeparation);
    setMaterial();*/
}
Vec3f VRView::getProjectionUser() { return proj_user; }

OSG_END_NAMESPACE;
