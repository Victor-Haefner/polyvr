#include "VRGeoPrimitive.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/objects/geometry/VRHandle.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include <boost/bind.hpp>

using namespace OSG;

VRGeoPrimitive::VRGeoPrimitive(string name) : VRGeometry(name) {
    type = "GeoPrimitive";
}

VRGeoPrimitivePtr VRGeoPrimitive::create(string name) {
    auto p = VRGeoPrimitivePtr( new VRGeoPrimitive(name) );
    p->setPrimitive("Box");
    return p;
}

VRGeoPrimitivePtr VRGeoPrimitive::ptr() { return static_pointer_cast<VRGeoPrimitive>( shared_from_this() ); }

void VRGeoPrimitive::select(bool b) {
    if (selected == b) return;
    selected = b;
    for (auto h : handles) h->setVisible(b);
}

void VRGeoPrimitive::update(int i, float v) {
    if (!primitive) return;
    auto params = splitString(primitive->toString(), ' ');
    string args;
    for (int j=0; j<params.size(); j++) {
        if (i != j) args += params[j];
        else args += toString(v);
        if (j < params.size()-1) args += " ";
    }
    VRGeometry::setPrimitive(primitive->getType(), args);
}

void VRGeoPrimitive::setupHandles() {
    for (auto h : handles) subChild(h);
    handles.clear();

    string type = primitive->getType();
    int N = primitive->getNParams();
    auto params = splitString(primitive->toString(), ' ');
    auto param_names = VRPrimitive::getTypeParameter(type);

    for (int i=0; i<N; i++) {
        string n = param_names[i];
        string param = params[i];

        auto h = VRHandle::create(n);
        handles.push_back(h);
        addChild(h);

        auto cb = VRFunction<float>::create( "geo_prim_update", boost::bind(&VRGeoPrimitive::update, this, i, _1) );

        if (n == "Size x") h->configure(cb, VRHandle::LINEAR, Vec3f(1,0,0), 0.5, true);
        if (n == "Size y") h->configure(cb, VRHandle::LINEAR, Vec3f(0,1,0), 0.5, true);
        if (n == "Size z") h->configure(cb, VRHandle::LINEAR, Vec3f(0,0,1), 0.5, true);
        if (n == "Radius") h->configure(cb, VRHandle::LINEAR, Vec3f(1,0,0), 1, true);
        if (n == "Height") h->configure(cb, VRHandle::LINEAR, Vec3f(0,1,0), 0.5, true);
        if (n == "Inner radius") h->configure(cb, VRHandle::LINEAR, Vec3f(0,1,0), 1, true);
        if (n == "Outer radius") h->configure(cb, VRHandle::LINEAR, Vec3f(1,0,0), 1, true);

        h->set( pose(), toFloat(param) );
    }
}

void VRGeoPrimitive::setPrimitive(string prim, string args) {
    VRGeometry::setPrimitive(prim, args);
    setupHandles(); // change primitive type
    select(true);
}
