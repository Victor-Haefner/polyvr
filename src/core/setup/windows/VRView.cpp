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
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRSprite.h"
#include "core/objects/VRTransform.h"
#include "core/objects/VRCamera.h"
#include "core/objects/object/VRObjectT.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

bool onBox(int i, int j, int c) {
    if(abs(i) > c || abs(j) > c) return false;
    if(abs(i) == c || abs(j) == c) return true;
    return false;
}

void VRView::setMaterial() {
    ImageRecPtr img = Image::create();

    Vec4f bg = Vec4f(0.5, 0.7, 0.95, 0.5);
    Vec4f c1 = Vec4f(0.5, 0.7, 0.95, 0.9);
    Vec4f cax = Vec4f(0.9, 0.2, 0.2, 1);
    Vec4f cay = Vec4f(0.2, 0.9, 0.2, 1);

    ImageRecPtr label = VRText::get()->create(name, "SANS 20", 20, Color4f(0,0,0,255), Color4f(bg[2]*255.0, bg[1]*255.0, bg[0]*255.0, 0));
    float lw = label->getWidth();
    float lh = label->getHeight();

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
                const UInt8* d = label->getData();
                data[k] = Vec4f(d[w]/255.0, d[w+1]/255.0, d[w+2]/255.0, d[w+3]/255.0);
                //data[k] = Vec4f(1,1,1, 1);
            }
        }
    }

    img->set( Image::OSG_RGBA_PF, s, s, 1, 0, 1, 0, (const uint8_t*)&data[0], OSG::Image::OSG_FLOAT32_IMAGEDATA, true, 1);

    viewGeoMat->setTexture(img);
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

        screenLowerLeft = Pnt3f(proj_center - proj_up*h*0.5 + x*w*0.5);
        screenLowerRight = Pnt3f(proj_center - proj_up*h*0.5 - x*w*0.5);
        screenUpperRight = Pnt3f(proj_center + proj_up*h*0.5 - x*w*0.5);
        screenUpperLeft = Pnt3f(proj_center + proj_up*h*0.5 + x*w*0.5);
    } else {
        screenLowerLeft = Pnt3f(-1, -0.6, -1);
        screenLowerRight = Pnt3f(1,-0.6, -1);
        screenUpperRight = Pnt3f(1,0.6, -1);
        screenUpperLeft = Pnt3f(-1,0.6, -1);
    }

    GeometryRecPtr geo = dynamic_cast<Geometry*>(viewGeo->getCore());
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

//VRView::VRView(bool _active_stereo, bool _stereo, bool _projection, Pnt3f _screenLowerLeft, Pnt3f _screenLowerRight, Pnt3f _screenUpperRight, Pnt3f _screenUpperLeft, bool swapeyes) {
VRView::VRView(string n) {
    // pointer
    lView = 0;
    rView = 0;
    lView_act = 0;
    rView_act = 0;
    PCDecoratorLeft = 0;
    PCDecoratorRight = 0;
    view_root = 0;
    cam = 0;
    real_root = 0;
    user = 0;
    viewGeo = 0;
    name = n;
    window = 0;

    // flags
    eyeinverted = false;
    doStats = false;
    active_stereo = false;
    stereo = false;
    projection = false;

    // data
    position = Vec4f(0,0,1,1);
    proj_center = Vec3f(0,0,-1);
    proj_up = Vec3f(0,1,0);
    proj_normal = Vec3f(0,0,1);
    proj_size = Vec2f(2,1);

    //if (active_stereo) setActiveViewports();
    //else setPassiveViewports();

    SolidBackgroundRecPtr sbg = SolidBackground::create();
    sbg->setColor(Color3f(0.7, 0.7, 0.7));
    background = sbg;

    eyeSeparation = 0.06;

    stats = 0;
    grabfg = 0;
    dummy_user = new VRTransform("view_user");
    dummy_user->addAttachment("global", 0);

    viewGeo = makeNodeFor(makePlaneGeo(1,1,1,1));
    viewGeo->setTravMask(0);
    viewGeoMat = new VRMaterial("setup view mat");
    GeometryRecPtr geo = dynamic_cast<Geometry*>( viewGeo->getCore() );
    geo->setMaterial(viewGeoMat->getMaterial());
    update();
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
    delete dummy_user;
}

int VRView::getID() { return ID; }
void VRView::setID(int i) { ID = i; }

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
void VRView::setPosition(Vec4f pos) {
    position = pos;
    setViewports();
}

void VRView::setRoot(VRObject* root, VRTransform* real) { view_root = root; real_root = real; update(); }

void VRView::setRoot() {
    if (real_root && viewGeo) real_root->addChild(viewGeo);

    if (user && real_root) user->switchParent(real_root);
    if (dummy_user && real_root) dummy_user->switchParent(real_root);

    NodeRecPtr n = view_root ? view_root->getNode() : 0;
    if (lView) lView->setRoot(n);
    if (rView) rView->setRoot(n);
}

void VRView::setUser(VRTransform* u) {
    user = u;
    user_name = user ? user->getName() : "";
    update();
}

void VRView::setUser() {
    if (user == 0 && user_name != "") user = VRSetupManager::getCurrent()->getTracker(user_name);

    if (user == 0) {
        if (PCDecoratorLeft) PCDecoratorLeft->setUser(dummy_user->getNode());
        if (PCDecoratorRight) PCDecoratorRight->setUser(dummy_user->getNode());
    } else {
        user_name = user->getName();
        if (PCDecoratorLeft) PCDecoratorLeft->setUser(user->getNode());
        if (PCDecoratorRight) PCDecoratorRight->setUser(user->getNode());
    }
}

void VRView::setCamera(VRCamera* c) { cam = c; update(); }

void VRView::setCam() {
    if (cam == 0) return;

    if (lView && PCDecoratorLeft == 0) lView->setCamera(cam->getCam());
    if (rView && PCDecoratorRight == 0) rView->setCamera(cam->getCam());

    if (PCDecoratorLeft) PCDecoratorLeft->setDecoratee(cam->getCam());
    if (PCDecoratorRight) PCDecoratorRight->setDecoratee(cam->getCam());

    if (lView && PCDecoratorLeft) lView->setCamera(PCDecoratorLeft);
    if (rView && PCDecoratorRight) rView->setCamera(PCDecoratorRight);
}

void VRView::setBackground(BackgroundRecPtr bg) { background = bg; update(); }

void VRView::setWindow(WindowRecPtr win) { window = win; }

void VRView::setWindow() {
    if (window == 0) return;
    if (lView) window->addPort(lView);
    if (rView) window->addPort(rView);
}

void VRView::setStereo(bool b) { stereo = b; update(); }

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
        NodeRecPtr n = Node::create();
        if (rView) rView->setRoot(n);*/
        if (PCDecoratorLeft) PCDecoratorLeft->setEyeSeparation(0);
        if (PCDecoratorRight) PCDecoratorRight->setEyeSeparation(0);
    } else update();
}

void VRView::setCallibrationMode(bool b) {
    if (b) {
        typedef OSG::Vector< OSG::UInt8, 4 > Vec4c;
        int w = window->getWidth(); // TODO: get the right window size from server window
        int h = window->getHeight();


        if (w*h <= 0) return;

        vector<Vec4c> data(w*h);
        Vec4c c1(0,0,0,255);
        Vec4c c2(255,255,255,255);

        int w1 = 0.1*w;
        int h1 = 0.1*h;
        int w5 = 0.5*w;
        int h5 = 0.5*h;

        for (int i=0; i<w; i++) {
            for (int j=0; j<h; j++) {
                int x = i-w5;
                int y = j-h5;
                int l = sqrt(x*x+y*y);
                int k = i+j*w;

                data[k] = c1;
                if (i == 0 || j == 0 || i == w-1 || j == h-1) data[k] = c2;
                else if (i == w1 || j == w1 || i == w-w1 || j == h-h1) data[k] = c2;
                else if (x == -h5 || y == -w5 || x == h5 || y == w5) data[k] = c2;
                else if(l == h5 || l == w5 || l == w1) data[k] = c2;
                else if(x == 0 || y == 0) data[k] = c2;
            }
        }

        ImageRecPtr img = Image::create();
        img->set(Image::OSG_RGBA_PF, w, h, 1, 0, 1, 0, (const uint8_t*)&data[0], Image::OSG_UINT8_IMAGEDATA, true, 1);

        calib_fg = ImageForeground::create();
        calib_fg->addImage(img, Pnt2f());
        if (lView) lView->addForeground(calib_fg);
        if (rView) rView->addForeground(calib_fg);
    } else {
        //if (lView) lView->removeObjFromForegrounds(calib_fg); // TODO
        //if (rView) rView->removeObjFromForegrounds(calib_fg);
        if (lView) lView = 0; // WORKAROUND
        if (rView) rView = 0;
        if (lView_act) lView_act = 0;
        if (rView_act) rView_act = 0;
        update();
    }
}

ImageRecPtr VRView::grab() {
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
    node->set_attribute("up", toString(proj_up).c_str());
    node->set_attribute("size", toString(proj_size).c_str());
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
    proj_up = toVec3f(node->get_attribute("up")->get_value());
    proj_size = toVec2f(node->get_attribute("size")->get_value());
    if (node->get_attribute("user")) {
        user_name = node->get_attribute("user")->get_value();
        user = VRSetupManager::getCurrent()->getTracker(user_name);
    }

    showStats(doStats);
    update();
}

VRTransform* VRView::getUser() { if (user) return user; else return dummy_user; }
VRCamera* VRView::getCamera() { return cam; }
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

OSG_END_NAMESPACE;
