#include "VRGeoPrimitive.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/objects/geometry/VRHandle.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/tools/selection/VRSelector.h"
#include "VRAnnotationEngine.h"

using namespace OSG;


VRGeoPrimitive::VRGeoPrimitive(string name) : VRTransform(name) {
    type = "GeoPrimitive";
    selector = VRSelector::create();

    params_geo = VRAnnotationEngine::create();
    params_geo->getMaterial()->setLit(0);
    params_geo->setSize(0.015);
    params_geo->setBillboard(1);
    params_geo->setScreensize(1);
    params_geo->setColor(Color4f(0,0,0,1));
    params_geo->setBackground(Color4f(1,1,1,1));
    params_geo->getMaterial()->setDepthTest(GL_ALWAYS);
    params_geo->setPersistency(0);
}

VRGeoPrimitivePtr VRGeoPrimitive::create(string name) {
    auto p = VRGeoPrimitivePtr( new VRGeoPrimitive(name) );
    p->addChild(p->params_geo);
    p->setPrimitive("Box");
    return p;
}

VRGeoPrimitivePtr VRGeoPrimitive::ptr() { return static_pointer_cast<VRGeoPrimitive>( shared_from_this() ); }

VRAnnotationEnginePtr VRGeoPrimitive::getLabels() { return params_geo; }

void VRGeoPrimitive::setGeometry(VRGeometryPtr geo) {
    if (ownsGeometry) if (auto g = geometry.lock()) g->destroy();
    geometry = geo;
    ownsGeometry = false;
    setupHandles();
}

void VRGeoPrimitive::setHandleSize(float s) {
    size = s;
    for (auto h : handles) h->setSize(s);
}

VRHandlePtr VRGeoPrimitive::getHandle(int i) {
    if (i < 0 || i >= int(handles.size())) return 0;
    return handles[i];
}

void VRGeoPrimitive::select(bool b) {
    if (selected == b) return;
    selected = b;
    params_geo->setVisible(b);
    for (auto h : handles) h->setVisible(b);

    if (!selector) return;
    if (b) selector->select(ptr(), false, false); // TODO: does not play nice with params_geo!!
    else selector->clear();
}

void VRGeoPrimitive::update(int i, VRHandleWeakPtr hw, float v) {
    VRGeometryPtr geo = geometry.lock();
    if (!geo) return;
    auto primitive = geo->getPrimitive();
    if (!primitive) return;

    auto params = splitString(primitive->toString(), ' ');
    string args;
    for (unsigned int j=0; j<params.size(); j++) {
        if (i != int(j)) args += params[j];
        else args += toString(v);
        if (j < params.size()-1) args += " ";
    }
    geo->setPrimitive(primitive->getType() + " " + args);

    auto h = hw.lock();
    if (!params_geo || !h) return;
    auto a = h->getAxis();
    auto o = h->getOrigin()->pos();
    string lbl = h->getBaseName() + " " + toString( v*1000, 4 ) + " mm";
    params_geo->set(i, (a*v*0.5 + o)*0.5, lbl);
}

void VRGeoPrimitive::setupHandles() {
    VRGeometryPtr geo = geometry.lock();
    if (!geo) return;
    auto primitive = geo->getPrimitive();
    if (!primitive) return;

    for (auto h : handles) h->destroy();
    handles.clear();
    params_geo->clear();

    string type = primitive->getType();
    int N = primitive->getNParams();
    auto params = splitString(primitive->toString(), ' ');
    auto param_names = VRPrimitive::getTypeParameter(type);

    for (int i=0; i<N; i++) {
        string n = param_names[i];
        string param = params[i];

        if (n == "Segments x") continue;
        if (n == "Segments y") continue;
        if (n == "Segments z") continue;
        if (n == "Iterations") continue;
        if (n == "Do bottom") continue;
        if (n == "Do top") continue;
        if (n == "Do sides") continue;
        if (n == "Sides") continue;
        if (n == "Segments") continue;
        if (n == "Rings") continue;
        if (n == "Number of teeth") continue;

        function<VRHandlePtr(Vec3d, float, bool)> addHandle = [&](Vec3d d, float L, bool symmetric) {
            auto h = VRHandle::create(n);
            h->setSize(size);
            h->setPersistency(0);
            handles.push_back(h);
            geo->addChild(h);

            auto cb = VRFunction<float>::create( "geo_prim_update", bind(&VRGeoPrimitive::update, this, i, h, placeholders::_1) );
            h->configure(cb, VRHandle::LINEAR, d, L);

            float v = toFloat(param);
            h->set( Pose::create(), v );
            string lbl = h->getBaseName() + " " + toString( v*1000, 4 ) + " mm";
            auto a = h->getAxis();
            auto o = h->getOrigin()->pos();
            params_geo->set(i, (a*v*0.5 + o)*0.5, lbl);

            if (symmetric) {
                auto h2 = addHandle(-d,L,false);
                h->addSibling(h2);
                h2->addSibling(h);
            }
            return h;
        };

        if (n == "Scale") addHandle(Vec3d(1,0,0), 1, false);
        if (n == "Size x") addHandle(Vec3d(1,0,0), 0.5, true);
        if (n == "Size y") addHandle(Vec3d(0,1,0), 0.5, true);
        if (n == "Size z") addHandle(Vec3d(0,0,1), 0.5, true);
        if (n == "Radius") addHandle(Vec3d(1,0,0), 1, false);
        if (n == "Height" && type != "Arrow") addHandle(Vec3d(0,1,0), 0.5, false);
        if (n == "Height" && type == "Arrow") addHandle(Vec3d(0,0,1), 1, false);
        if (n == "Width") addHandle(Vec3d(1,0,0), 0.5, false);
        if (n == "Trunc") addHandle(Vec3d(1,0,0), 0.5, false);
        if (n == "Hat") addHandle(Vec3d(0,0,1), 1, false);
        if (n == "Inner radius") addHandle(Vec3d(0,1,0), 1, false);
        if (n == "Outer radius") addHandle(Vec3d(1,0,0), 0.5, false);
        if (n == "Hole") addHandle(Vec3d(1,0,0), 1, false);
        if (n == "Pitch") addHandle(Vec3d(0,0,1), 2, false);
        if (n == "Teeth size") addHandle(Vec3d(0,1,0), 1, false);
        if (n == "Bevel") addHandle(Vec3d(1,1,0), 1, false);
        if (n == "Length") addHandle(Vec3d(0,0,1), 1, false);
    }
}

void VRGeoPrimitive::setPrimitive(string params) {
    VRGeometryPtr geo = geometry.lock();
    if (!geo) {
        geo = VRGeometry::create(name+"_geo");
        addChild(geo);
        geometry = geo;
        ownsGeometry = true;
    }
    geo->setPrimitive(params);
    setupHandles(); // change primitive type
    select(true);
}

vector<VRHandlePtr> VRGeoPrimitive::getHandles() { return handles; }
