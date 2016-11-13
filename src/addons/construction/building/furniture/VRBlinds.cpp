#include "VRBlinds.h"

#include "core/scene/sound/VRSoundManager.h"
#include "core/scene/VRAnimationManagerT.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include <OpenSG/OSGGeoProperties.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRObjectPtr VRBlinds::copy(vector<VRObjectPtr> children) {
    VRBlindsPtr d = VRBlinds::create(getBaseName(), window, scene);

    d->state = state;
    d->sound = sound;

    return d;
}

VRBlinds::VRBlinds(string name, VRGeometryPtr _window, VRScene* _scene): VRTransform(name) {
    state = OPEN;
    scene = _scene;

    window = _window;
    window->hide();

    create();

    blend_geo->addAttachment("blind", 0);

    // animation callback
    fkt = VRFunction<float>::create("Blinds_interpolate", boost::bind(&VRBlinds::interpolate, this, _1));

    // toggle callback
    toggleCallback = VRFunction<VRDeviceWeakPtr>::create("Blinds_toggle", boost::bind(&VRBlinds::toggle, this, _1));
}

VRBlindsPtr VRBlinds::create(string name, VRGeometryPtr _window, VRScene* _scene) {
    return shared_ptr<VRBlinds>( new VRBlinds(name, _window, _scene) );
}

VRBlindsPtr VRBlinds::ptr() { return static_pointer_cast<VRBlinds>( shared_from_this() ); }

void VRBlinds::setSound(string s) { sound = s; }

bool VRBlinds::isClose() { return (state == CLOSE); }
bool VRBlinds::isOpen() { return (state == OPEN); }

void VRBlinds::open() {
    if (state == OPEN) return;
    state = OPEN;

    scene->addAnimation<float>(3, 0, fkt, float(0.0), float(1.0), false);
    VRSoundManager::get().playSound(sound);
}

void VRBlinds::close() {
    if (state == CLOSE) return;
    state = CLOSE;

    scene->addAnimation<float>(3, 0, fkt, float(1.0), float(0.0), false);
    VRSoundManager::get().playSound(sound);
}

void VRBlinds::toggle(VRDeviceWeakPtr d) {
    if (auto dev = d.lock()) { //if triggered by a device, check if this is hit
        VRIntersection ins = dev->intersect(ptr());
        if (!ins.hit) return;
        if ( ins.object.lock() == 0 ) return;
        if ( !ins.object.lock()->hasAncestorWithAttachment("blind")) return;
    }

    if (state == CLOSE) open();
    else close();
}


void VRBlinds::create() {
    vector<Vec3f> norms;
    vector<int> inds;
    vector<Vec2f> texs;

    for (int i=0;i<20;i++) {//20 blend elements
        bl_pos_closed.push_back(Vec3f(0.55, -0.07*i, 0));
        bl_pos_closed.push_back(Vec3f(0.55, -0.07*(i+0.3), -0.04));
        bl_pos_closed.push_back(Vec3f(0.55, -0.07*(i+0.9), -0.08));

        bl_pos_closed.push_back(Vec3f(-0.55, -0.07*(i+0.9), -0.08));
        bl_pos_closed.push_back(Vec3f(-0.55, -0.07*(i+0.3), -0.04));
        bl_pos_closed.push_back(Vec3f(-0.55, -0.07*i, 0));

        bl_pos_open.push_back(Vec3f(0.55, -0.01*i, 0));
        bl_pos_open.push_back(Vec3f(0.55, -0.01*(i-0.7), -0.05));
        bl_pos_open.push_back(Vec3f(0.55, -0.01*i, -0.1));

        bl_pos_open.push_back(Vec3f(-0.55, -0.01*i, -0.1));
        bl_pos_open.push_back(Vec3f(-0.55, -0.01*(i-0.7), -0.05));
        bl_pos_open.push_back(Vec3f(-0.55, -0.01*i, 0));

        inds.push_back(i*6+0);//quad1
        inds.push_back(i*6+1);//quad1
        inds.push_back(i*6+4);//quad1
        inds.push_back(i*6+5);//quad1

        for (int j=1;j<5;j++) inds.push_back(i*6+j);//quad2

        for (int j=0;j<6;j++) norms.push_back(Vec3f(0,1,0));

        texs.push_back(Vec2f(1,0));
        texs.push_back(Vec2f(1,0.5));
        texs.push_back(Vec2f(1,1));

        texs.push_back(Vec2f(0,1));
        texs.push_back(Vec2f(0,0.5));
        texs.push_back(Vec2f(0,0));
    }

    blend_geo = VRGeometry::create("blend");
    blend_geo->create(GL_QUADS, bl_pos_open, norms, inds, texs);

    Vec3f pos = window->getGeometricCenter();
    Vec3f norm = window->getAverageNormal();
    norm.normalize();
    pos[2] = window->getMax(2);

    Matrix m = window->getWorldMatrix();
    m.mult(pos, pos);
    m.mult(norm, norm);

    blend_geo->setPose(pos, norm, Vec3f(0,1,0));
    scene->add(blend_geo);

    auto mat = VRMaterial::create("blinds");
    mat->setDiffuse(Color3f(0.5,0.5,0.5));
    mat->setAmbient(Color3f(0.2, 0.2, 0.2));
    mat->setSpecular(Color3f(0.1, 0.1, 0.1));
    blend_geo->setMaterial(mat);
}

void VRBlinds::interpolate(float t) {
    GeoPnt3fPropertyRecPtr pos;
    for (uint i=0; i<bl_pos_open.size(); i++)
        pos->addValue( (bl_pos_open[i]-bl_pos_closed[i])*t );
    blend_geo->setPositions(pos);
}

VRDeviceCbPtr VRBlinds::getCallback() { return toggleCallback; }

OSG_END_NAMESPACE;
