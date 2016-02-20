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
    p1 = p2;
    p2 = p3;
    p3 = p;
    update();
}

void VRMeasure::update() {
    Vec3f v1,v2,v3, n1,n2,n3;
    v1 = p3-p2; v2 = p1-p3; v3 = p2-p1;
    n1 = v1; n2 = v2; n3 = v3;
    n1.normalize(); n2.normalize(); n3.normalize();

    float a1 = acos(n2.dot(-n3))*180/Pi;
    float a2 = acos(n1.dot(-n3))*180/Pi;
    float a3 = acos(n1.dot(-n2))*180/Pi;

    Vec3f r(1,0,0);
    Vec3f g(0,1,0);
    Vec3f y(1,1,0);

    setVector(0, p1, v3, r, "a: " + toString( (p2-p1).length(), 4 ) + " m");
    setVector(1, p2, v1, g, "b: " + toString( (p2-p3).length(), 4 ) + " m");
    setVector(2, p3, v2, y, "c: " + toString( (p1-p3).length(), 4 ) + " m");
    setAngle(3, p1, -v2, v3, y,r, "i: " + toString( a1, 4 ) + " deg");
    setAngle(4, p2, v1, -v3, g,r, "j: " + toString( a2, 4 ) + " deg");
    setAngle(5, p3, -v1, v2, g,y, "k: " + toString( a3, 4 ) + " deg");
}

OSG_END_NAMESPACE
