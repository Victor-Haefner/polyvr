#include "VRMesure.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRSprite.h"
#include "core/objects/geometry/VRBillboard.h"
#include "core/objects/VRCamera.h"
#include "core/tools/VRText.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"

#include <OpenSG/OSGGeoProperties.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

string VRMesure::convertToString(float f, int p) {
    stringstream stream;
    stream << fixed;
    stream << setprecision(p);
    stream << f;
    return stream.str();
}

void VRMesure::processBar(Vec3f p1, Vec3f p2) {
    //p1 -= l->getWorldPosition();
    //p2 -= l->getWorldPosition();
    Vec3f d = p2-p1;

    Vec3f t1, t2;
    if (d[1] < d[0]) t1 = d.cross(Vec3f(0,1,0));//take the biggest axis
    else t1 = d.cross(Vec3f(1,0,0));
    t2 = d.cross(t1);

    t1.normalize();
    t1 *= 0.01;
    t2.normalize();
    t2 *= 0.01;

    GeometryRecPtr geo = l->getMesh();
    GeoPnt3fPropertyRecPtr pos = dynamic_cast<GeoPnt3fProperty*>(geo->getPositions());
    GeoVec3fPropertyRecPtr norms = dynamic_cast<GeoVec3fProperty*>(geo->getNormals());

    pos->setValue(p1+t1, 5);
    pos->setValue(p1+t1, 12);
    pos->setValue(p1+t1, 20);

    pos->setValue(p1+t2, 4);
    pos->setValue(p1+t2, 9);
    pos->setValue(p1+t2, 21);

    pos->setValue(p1-t1, 6);
    pos->setValue(p1-t1, 11);
    pos->setValue(p1-t1, 19);

    pos->setValue(p1-t2, 7);
    pos->setValue(p1-t2, 14);
    pos->setValue(p1-t2, 18);

    pos->setValue(p2+t1, 0);
    pos->setValue(p2+t1, 13);
    pos->setValue(p2+t1, 22);

    pos->setValue(p2+t2, 1);
    pos->setValue(p2+t2, 8);
    pos->setValue(p2+t2, 23);

    pos->setValue(p2-t1, 3);
    pos->setValue(p2-t1, 10);
    pos->setValue(p2-t1, 17);

    pos->setValue(p2-t2, 2);
    pos->setValue(p2-t2, 15);
    pos->setValue(p2-t2, 16);

    norms->setValue(p1+t1, 5);
    norms->setValue(p1+t1, 12);
    norms->setValue(p1+t1, 20);

    norms->setValue(p1+t2, 4);
    norms->setValue(p1+t2, 9);
    norms->setValue(p1+t2, 21);

    norms->setValue(p1-t1, 6);
    norms->setValue(p1-t1, 11);
    norms->setValue(p1-t1, 19);

    norms->setValue(p1-t2, 7);
    norms->setValue(p1-t2, 14);
    norms->setValue(p1-t2, 18);

    norms->setValue(p2+t1, 0);
    norms->setValue(p2+t1, 13);
    norms->setValue(p2+t1, 22);

    norms->setValue(p2+t2, 1);
    norms->setValue(p2+t2, 8);
    norms->setValue(p2+t2, 23);

    norms->setValue(p2-t1, 3);
    norms->setValue(p2-t1, 10);
    norms->setValue(p2-t1, 17);

    norms->setValue(p2-t2, 2);
    norms->setValue(p2-t2, 15);
    norms->setValue(p2-t2, 16);
}

void VRMesure::processLabel(Vec3f p1, Vec3f p2, Vec3f cpos) {
    Vec3f d = p2-p1;

    Vec3f mitte = p1+d*0.5;//mitte vom strang
    display->setFrom(mitte);
    display->setAt(mitte*2 - cpos);

    Color4f bg = Color4f(50, 50, 50,255);
    Color4f fg = Color4f(255,255,255,255);

    string number = convertToString(d.length(), 2);
    float scale = 3;
    ImageRecPtr texture = VRText::get()->create(number, "SANS 20", 20, fg, bg);
    display->setSize(0.2*number.size()*scale, 0.3*scale);
    display->setTexture(texture);
}

void VRMesure::check() {//check spheres for change of position
    Vec3f p1, p2;

    VRCamera* cam = scene->getActiveCamera();
    VRTransform* user = VRSetupManager::getCurrent()->getUser();

    p1 = s1->getWorldPosition();
    p2 = s2->getWorldPosition();

    processBar(p1, p2);
    if (user == 0) processLabel(p1, p2, cam->getWorldPosition());
    else processLabel(p1, p2, user->getWorldPosition());
}

MaterialRecPtr VRMesure::setTransMat() {
    SimpleMaterialRecPtr mat = SimpleMaterial::create();
    mat->setDiffuse(Color3f(0.7,0.7,0.7));
    mat->setAmbient(Color3f(0.2, 0.2, 0.2));
    mat->setTransparency(0.7);
    return mat;
}

void VRMesure::_kill() {
    scene->dropUpdateFkt(fkt);
    s1->hide();
    s2->hide();
    l->hide();
}

VRMesure::VRMesure() {
    s1 = new VRGeometry("mesure_s1");//unique names?
    s2 = new VRGeometry("mesure_s2");
    l = new VRGeometry("mesure_l");
    display = new VRBillboard("ecoflex_nametag", false);

    s1->setPickable(true);
    s2->setPickable(true);

    s1->setPrimitive("Sphere", "3 0.2");
    s2->setPrimitive("Sphere", "3 0.2");
    l->setPrimitive("Box", "0.1 0.1 0.1 1 1 1");
    l->addChild(display);

    MaterialRecPtr tm = setTransMat();
    s1->setMaterial(tm);
    s2->setMaterial(tm);

    Color4f bg = Color4f(50, 50, 50,255);
    Color4f fg = Color4f(255,255,255,255);

    string s = "test";
    float scale = 1;
    ImageRecPtr texture = VRText::get()->create(s, "SANS 20", 20, fg, bg);
    display->setSize(0.2*s.size()*scale, 0.3*scale);
    display->setTexture(texture);

    fkt = new VRFunction<int>("Mesure_check", boost::bind(&VRMesure::check, this));
}

void VRMesure::setKillSignal(VRDevice* dev, VRSignal* sig) {
    VRFunction<VRDevice*>* cb = new VRFunction<VRDevice*>("Mesure_kill", boost::bind(&VRMesure::kill, this, _1));
    sig->add( cb );
}

void VRMesure::addToScene(VRScene* _scene) {
    scene = _scene;
    scene->add(s1);
    scene->add(s2);
    scene->add(l);
    scene->addUpdateFkt(fkt);
}

void VRMesure::setPosition(Vec3f pos) {
    s1->setFrom(pos);
    s2->setFrom(pos);
}

void VRMesure::kill(VRDevice* dev) {
    if (dev == 0) { _kill(); return; }

    VRIntersection ins = dev->intersect(l);
    if (!ins.hit) return;
    if (ins.object == 0) return;
    if (ins.object->hasAncestor(s1) || ins.object->hasAncestor(s2)) _kill();
}

OSG_END_NAMESPACE
