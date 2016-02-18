#include "VRMesure.h"
#include "core/utils/toString.h"


OSG_BEGIN_NAMESPACE;
using namespace std;

VRMeasure::VRMeasure(string name) {
    setName(name);
    setLabelParams(0.05, true, true);
}

VRMeasurePtr VRMeasure::ptr() { return static_pointer_cast<VRMeasure>( shared_from_this() ); }
VRMeasurePtr VRMeasure::create(string name) {
    auto ptr = shared_ptr<VRMeasure>(new VRMeasure(name) );
    ptr->init();
    return ptr;
}

void VRMeasure::setPoint(int i, Vec3f p) {
    if (i == 0) p1 = p;
    if (i == 1) p2 = p;
    if (i == 2) p3 = p;
    update();
}

void VRMeasure::rollPoints(Vec3f p) {
    cout << "VRMeasure::rollPoints " << p << endl;
    p1 = p2;
    p2 = p3;
    p3 = p;
    update();
}

void VRMeasure::update() {
    cout << "VRMeasure::update " << p1 << ", " << p2 << ", " << p3 << endl;
    setVector(0, p1, p2-p1, Vec3f(1,0,0), "a: " + toString( (p2-p1).length(), 4 ) + " m");
    setVector(1, p3, p2-p3, Vec3f(0,1,0), "b: " + toString( (p2-p3).length(), 4 ) + " m");
    setVector(2, p3, p1-p3, Vec3f(1,1,0), "c: " + toString( (p1-p3).length(), 4 ) + " m");
}

OSG_END_NAMESPACE
