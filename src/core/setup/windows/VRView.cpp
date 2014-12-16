#include "VRView.h"
#include <OpenSG/OSGRenderAction.h>
#include <OpenSG/OSGImageForeground.h>
#include <libxml++/nodes/element.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.

#include "core/utils/VRRate.h"
#include "core/utils/toString.h"
#include "core/tools/VRText.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/gui/VRGuiManager.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRSprite.h"
#include "core/objects/VRTransform.h"
#include "core/objects/VRCamera.h"
#include "core/objects/object/VRObjectT.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

bool onBox(int i, int j, int c) {
    if(abs(i) > c or abs(j) > c) return false;
    if(abs(i) == c or abs(j) == c) return true;
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

    Vec4f data[s*s];

    for (int i=0; i<s; i++) {
        for (int j=0; j<s; j++) {
            int k = i+j*s;
            data[k] = bg;

            int x = i-0.5*s;
            int y = j-0.5*s;

            if (onBox(x,y,b1)) data[k] = c1; // box1
            if (onBox(x,y,b2)) data[k] = c1; // box2

            if (y == 0 and x >= 0 and x < ar) data[k] = cax; // ax
            if (x == 0 and y >= 0 and y < ar) data[k] = cay; // ax

            if (x >= pl[0]-lw*0.5 and x < pl[0]+lw*0.5 and y >= pl[1]-lh*0.5 and y < pl[1]+lh*0.5) {
                int u = x - pl[0] + lw*0.5;
                int v = y - pl[1] + lh*0.5;
                int w = 4*(u+v*lw);
                const UInt8* d = label->getData();
                data[k] = Vec4f(d[w]/255.0, d[w+1]/255.0, d[w+2]/255.0, d[w+3]/255.0);
                //data[k] = Vec4f(1,1,1, 1);
            }
        }
    }

    img->set( Image::OSG_RGBA_PF, s, s, 1, 0, 1, 0, (const uint8_t*)data, OSG::Image::OSG_FLOAT32_IMAGEDATA, true, 1);

    viewGeoMat->setTexture(img);
    viewGeoMat->setLit(false);
}

void VRView::setViewports() {//create and set size of viewports
    Vec4f p = position;
    p[1] = 1-position[3]; // invert y
    p[3] = 1-position[1];

    if (p[0] > p[2]) p[0] = p[2]-0.01;
    if (p[1] > p[3]) p[1] = p[3]-0.01;


    //active, stereo
    if (active_stereo) {
        if (lView_act == 0) lView_act = StereoBufferViewport::create();
        if (rView_act == 0) rView_act = StereoBufferViewport::create();
    } else {
        lView_act = 0;
        rView_act = 0;
    }

    //no stereo
    if (!stereo and !active_stereo) {
        if (lView == 0) lView = Viewport::create();
        lView->setSize(p[0], p[1], p[2], p[3]);
        if (rView) window->subPortByObj(rView);
        rView = 0;
        return;
    }

    if (stereo and !active_stereo) {
        if (lView == 0) lView = Viewport::create();
        if (rView == 0) rView = Viewport::create();
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
        screenLowerLeft = Pnt3f(-1, -1, -1);
        screenLowerRight = Pnt3f(1, -1, -1);
        screenUpperRight = Pnt3f(1,  1, -1);
        screenUpperLeft = Pnt3f(-1,  1, -1);
    }

    GeometryRecPtr geo = dynamic_cast<Geometry*>(viewGeo->getCore());
    GeoVectorPropertyRecPtr pos = geo->getPositions();

    pos->setValue(screenLowerLeft, 0);
    pos->setValue(screenLowerRight, 1);
    pos->setValue(screenUpperLeft, 2);
    pos->setValue(screenUpperRight, 3);

    if (!projection and !stereo) {
        PCDecoratorLeft = 0;
        PCDecoratorRight = 0;
        return;
    }

    if (projection and !stereo) {
        cout << "\nset single projection decorator";
        if (PCDecoratorLeft == 0) PCDecoratorLeft = ProjectionCameraDecorator::create();
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
    if (PCDecoratorLeft == 0) PCDecoratorLeft = ProjectionCameraDecorator::create();
    if (PCDecoratorRight == 0) PCDecoratorRight = ProjectionCameraDecorator::create();

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

    // flags
    eyeinverted = false;
    doStats = false;
    active_stereo = false;
    stereo = false;
    projection = false;

    // data
    position = Vec4f(0,0,1,1);
    proj_center = Vec3f(0,0,0);
    proj_up = Vec3f(0,1,0);
    proj_normal = Vec3f(0,0,1);
    proj_size = Vec2f(2,1);

    //if (active_stereo) setActiveViewports();
    //else setPassiveViewports();

    SolidBackgroundRecPtr sbg = SolidBackground::create();
    sbg->setColor(Color3f(0.7, 0.7, 0.7));
    setBackground(sbg);

    eyeSeparation = 0.06;

    stats = 0;
    grabfg = 0;
    dummy_user = new VRTransform("view_user");
    dummy_user->translate(Vec3f(0,0,1));
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
        stats->addElement(RenderAction::statNStates, " State changes: %d");
        stats->addElement(RenderAction::statNShaders, "Shader changes: %d");
        stats->addElement(RenderAction::statNShaderParams, "Shader param changes: %d");
        stats->addElement(TextureObjChunk::statNTextures, "Textures: %d");
        stats->addElement(TextureObjChunk::statNTexBytes, " Tex Mem: %MB MB");


#if 0
        stats->addElement(RenderAction::statNGeometries,
                       "    Geom nodes: %d");
#endif
        stats->addElement(RenderAction::statNMatrices,
                       "Matrix changes: %d");
#if 0
        stats->addElement(RenderAction::statNTriangles,
                       "     Triangles: %d");
#endif

        /*stats->addElement(PointLight::statNPointLights,
                           "%d active point lights");
        stats->addElement(DirectionalLight::statNDirectionalLights,
                           "%d active directional lights");
        stats->addElement(SpotLight::statNSpotLights,
                           "%d active spot lights");*/

        stats->addText   ("Drawables: (drawn)");

        stats->addElement(Drawable::statNTriangles,    "  tris: %d");
        stats->addElement(Drawable::statNLines,        " lines: %d");
        stats->addElement(Drawable::statNPoints,       "points: %d");
        stats->addElement(Drawable::statNVertices,     " verts: %d");

        if(stats->getCollector() != NULL)
        {
            // add optional elements
            stats->getCollector()->getElem(Drawable::statNTriangles);
        }

        stats->addText   ("ChangeList: ");
        stats->addElement(ChangeList::statNChangedStoreSize,
                       "    %d entries in changedStore");
        stats->addElement(ChangeList::statNCreatedStoreSize,
                       "    %d entries in createdStore");
        stats->addElement(ChangeList::statNUnCommittedStoreSize,
                       "    %d entries in uncommitedStore");
        stats->addElement(ChangeList::statNPoolSize,
                       "    %d entries in pool");

#if 0
        // 1.x stat
        stats->addElement(RenderAction::statTravTime,
                           "FPS:                  %r.3f");
        stats->addElement(DrawActionBase::statCullTestedNodes,
                           "Nodes culltested      %d");
        stats->addElement(DrawActionBase::statCulledNodes,
                           "Nodes culled          %d");
        stats->addElement(RenderAction::statNOcclusionMode,
                           "Occlusion culling     %s");
        stats->addElement(RenderAction::statNOcclusionTests,
                           "Occlusion tests       %d");
        stats->addElement(RenderAction::statNOcclusionCulled,
                           "Occlusion culled      %d");
        stats->addElement(RenderAction::statNGeometries,
                           "Nodes drawn           %d");
        stats->addElement(RenderAction::statNTransGeometries,
                           "Transp. Nodes drawn   %d");
        stats->addElement(RenderAction::statNMaterials,
                           "Material changes      %d");
        stats->addElement(RenderAction::statNMatrices,
                           "Matrix changes        %d");

#if 0 // not ready for primetime yet
        stats->addElement(PointLight::statNPointLights,
                           "%d active point lights");
        stats->addElement(DirectionalLight::statNDirectionalLights,
                           "%d active directional lights");
        stats->addElement(SpotLight::statNSpotLights,
                           "%d active spot lights");
#endif
        stats->addElement(Drawable::statNTriangles,
                           "Triangles drawn       %d");
        stats->addElement(Drawable::statNLines,
                           "Lines drawn           %d");
        stats->addElement(Drawable::statNPoints,
                           "Points drawn          %d");
        stats->addElement(Drawable::statNVertices,
                           "Vertices transformed  %d");
        stats->addElement(RenderAction::statNTextures,
                           "Textures used         %d");
        stats->addElement(RenderAction::statNTexBytes,
                           "Textures size (bytes) %d");
#endif

#if 0
        // Render action
        stats->addElement(RenderAction::statDrawTime,      "Draw FPS: %r.3f");
        stats->addElement(RenderAction::statTravTime,      "Trav FPS: %r.3f");
        stats->addElement(DrawActionBase::statCullTestedNodes,
                           "%d Nodes culltested");
        stats->addElement(DrawActionBase::statCulledNodes,
                           "%d Nodes culled");
        stats->addElement(RenderAction::statNMaterials,
                           "%d material changes");
        stats->addElement(RenderAction::statNMatrices,
                           "%d matrix changes");
        stats->addElement(RenderAction::statNGeometries,
                           "%d Nodes drawn");
        stats->addElement(RenderAction::statNTransGeometries,
                           "%d transparent Nodes drawn");
#if 0 // not ready for primetime yet
        stats->addElement(PointLight::statNPointLights,
                           "%d active point lights");
        stats->addElement(DirectionalLight::statNDirectionalLights,
                           "%d active directional lights");
        stats->addElement(SpotLight::statNSpotLights,
                           "%d active spot lights");
#endif
        stats->addElement(Drawable::statNTriangles,    "%d triangles drawn");
        stats->addElement(Drawable::statNLines,        "%d lines drawn");
        stats->addElement(Drawable::statNPoints,       "%d points drawn");
        stats->addElement(Drawable::statNVertices,     "%d vertices transformed");
        stats->addElement(RenderAction::statNTextures, "%d textures used");
        stats->addElement(RenderAction::statNTexBytes, "%d bytes of texture used");
        if(stats->getCollector() != NULL)
        {
            // add optional elements
            stats->editCollector()->getElem(Drawable::statNTriangles);
        }
#endif

    }

    if (lView == 0) return;

    if (b and !doStats) lView->addForeground(stats);
    if (!b and doStats) lView->removeObjFromForegrounds(stats);

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

void VRView::setRoot(VRObject* root, VRTransform* real) {
    if (real) {
        real_root = real;
        real_root->addChild(viewGeo);
    }
    if (root) view_root = root;
    //if (view_root == 0) return;

    if (user and real_root) user->switchParent(real_root);
    if (dummy_user and real_root) dummy_user->switchParent(real_root);

    NodeRecPtr n = view_root ? view_root->getNode() : 0;
    if (lView) lView->setRoot(n);
    if (rView) rView->setRoot(n);
}

void VRView::setUser(VRTransform* u) {
    if (u) user = u;

    if (user == 0) {
        if (PCDecoratorLeft) PCDecoratorLeft->setUser(dummy_user->getNode());
        if (PCDecoratorRight) PCDecoratorRight->setUser(dummy_user->getNode());
    } else {
        if (PCDecoratorLeft) PCDecoratorLeft->setUser(user->getNode());
        if (PCDecoratorRight) PCDecoratorRight->setUser(user->getNode());
    }
}

void VRView::setCamera(VRCamera* c) {
    if (c) cam = c;
    if (cam == 0) return;

    if (lView and PCDecoratorLeft == 0) lView->setCamera(cam->getCam());
    if (rView and PCDecoratorRight == 0) rView->setCamera(cam->getCam());

    if (PCDecoratorLeft) PCDecoratorLeft->setDecoratee(cam->getCam());
    if (PCDecoratorRight) PCDecoratorRight->setDecoratee(cam->getCam());

    if (lView and PCDecoratorLeft) lView->setCamera(PCDecoratorLeft);
    if (rView and PCDecoratorRight) rView->setCamera(PCDecoratorRight);
}

void VRView::setBackground(BackgroundRecPtr bg) {
    if (bg) background = bg;
    if (background == 0) return;

    if (lView) lView->setBackground(background);
    if (rView) rView->setBackground(background);
}

void VRView::setWindow(WindowRecPtr win) {
    if (win) window = win;
    if (window == 0) return;

    if (lView) window->addPort(lView);
    if (rView) window->addPort(rView);
}

void VRView::setStereo(bool b) {
    stereo = b;
    update();
}

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
    if (lView) lView->editMFForegrounds()->clear();
    if (rView) rView->editMFForegrounds()->clear();
    setViewports();
    setDecorators();
    setCamera();
    setRoot();
    setUser();
    setWindow();
    setBackground();
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
    cout << "VRView::setCallibrationMode " << b << endl;
    if (b) {
        typedef OSG::Vector< OSG::UInt8, 4 > Vec4c;
        int w = window->getWidth();
        int h = window->getHeight();

        cout << "VRView::setCallibrationMode " << b << " " << w << " " << h << endl;

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
                if (i == 0 or j == 0 or i == w-1 or j == h-1) data[k] = c2;
                else if (i == w1 or j == w1 or i == w-w1 or j == h-h1) data[k] = c2;
                else if (x == -h5 or y == -w5 or x == h5 or y == w5) data[k] = c2;
                else if(l == h5 or l == w5 or l == w1) data[k] = c2;
                else if(x == 0 or y == 0) data[k] = c2;
            }
        }

        ImageRecPtr img = Image::create();
        img->set(Image::OSG_RGBA_PF, w, h, 1, 0, 1, 0, (const uint8_t*)&data[0], Image::OSG_UINT8_IMAGEDATA, true, 1);

        ImageForegroundRecPtr fg = ImageForeground::create();
        fg->addImage(img, Pnt2f());

        if (lView) lView->editMFForegrounds()->push_back(fg);
        if (rView) rView->editMFForegrounds()->push_back(fg);
    } else update();
}

ImageRecPtr VRView::grab() {
    if (grabfg == 0) {
        grabfg = GrabForeground::create();
        ImageRecPtr img = Image::create();
        grabfg->setImage(img);
        if (lView) lView->editMFForegrounds()->push_back(grabfg);
        //if (rView) rView->editMFForegrounds()->push_back(grabfg);
    }

    ImageRecPtr img = 0;
    if (lView) {
        grabfg->setActive(true);
        VRSetupManager::getCurrent()->updateWindows();
        VRGuiManager::get()->updateGtk();
        img = grabfg->getImage();
    }
    //if (rView) rView->editMFForegrounds()->push_back(grabfg);
    return img;
}

void VRView::save(xmlpp::Element* node) {
    node->set_attribute("stereo", toString(stereo).c_str());
    node->set_attribute("active_stereo", toString(active_stereo).c_str());
    node->set_attribute("projection", toString(projection).c_str());
    node->set_attribute("eye_inverted", toString(eyeinverted).c_str());
    node->set_attribute("eye_separation", toString(eyeSeparation).c_str());
    node->set_attribute("stats", toString(doStats).c_str());
    node->set_attribute("position", toString(position).c_str());
    node->set_attribute("center", toString(proj_center).c_str());
    node->set_attribute("normal", toString(proj_normal).c_str());
    node->set_attribute("up", toString(proj_up).c_str());
    node->set_attribute("size", toString(proj_size).c_str());
    if (user) node->set_attribute("user", user->getName().c_str());
}

void VRView::load(xmlpp::Element* node) {
    stereo = toBool(node->get_attribute("stereo")->get_value());
    active_stereo = toBool(node->get_attribute("active_stereo")->get_value());
    projection = toBool(node->get_attribute("projection")->get_value());
    eyeinverted = toBool(node->get_attribute("eye_inverted")->get_value());
    doStats = toBool(node->get_attribute("stats")->get_value());
    eyeSeparation = toFloat(node->get_attribute("eye_separation")->get_value());
    position = toVec4f(node->get_attribute("position")->get_value());
    proj_center = toVec3f(node->get_attribute("center")->get_value());
    proj_normal = toVec3f(node->get_attribute("normal")->get_value());
    proj_up = toVec3f(node->get_attribute("up")->get_value());
    proj_size = toVec2f(node->get_attribute("size")->get_value());
    if (node->get_attribute("user")) {
        string u = node->get_attribute("user")->get_value();
        user = VRSetupManager::getCurrent()->getTracker(u);
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
