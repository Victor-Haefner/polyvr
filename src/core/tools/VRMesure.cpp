#include "VRMesure.h"
#include "core/utils/toString.h"


using namespace OSG;


VRMeasure::VRMeasure(string name) : VRAnalyticGeometry(name) {
    setLabelParams(0.05, true, true);
}

VRMeasurePtr VRMeasure::ptr() { return static_pointer_cast<VRMeasure>( shared_from_this() ); }
VRMeasurePtr VRMeasure::create(string name) {
    auto ptr = shared_ptr<VRMeasure>(new VRMeasure(name) );
    ptr->init();
    return ptr;
}

void VRMeasure::setPoint(int i, PosePtr p) {
    if (i == 0) P1 = *p;
    if (i == 1) P2 = *p;
    if (i == 2) P3 = *p;
    update();
}

void VRMeasure::rollPoints(PosePtr p) {
    P1 = P2;
    P2 = P3;
    P3 = *p;
    update();
}

void VRMeasure::update() {
    Vec3d v1,v2,v3, vn1,vn2,vn3, p1,p2,p3, n1,n2,n3;
    p1 = P1.pos(); p2 = P2.pos(); p3 = P3.pos();
    n1 = P1.dir(); n2 = P2.dir(); n3 = P3.dir();
    v1 = p3-p2; v2 = p1-p3; v3 = p2-p1;
    vn1 = v1; vn2 = v2; vn3 = v3;
    vn1.normalize(); vn2.normalize(); vn3.normalize();
    n1.normalize(); n2.normalize(); n3.normalize();
    n1 *= n1.dot(v3);
    n2 *= n2.dot(v1);
    n3 *= n3.dot(v2);

    float a1 = acos(vn2.dot(-vn3))*180/Pi;
    float a2 = acos(vn1.dot(-vn3))*180/Pi;
    float a3 = acos(vn1.dot(-vn2))*180/Pi;

    Color3f r(1,0,0);
    Color3f g(0,1,0);
    Color3f y(1,1,0);

    setVector(0, p1, v3, r, "a: " + toString( v3.length()*1000, 4 ) + " mm");
    setVector(1, p2, v1, g, "b: " + toString( v1.length()*1000, 4 ) + " mm");
    setVector(2, p3, v2, y, "c: " + toString( v2.length()*1000, 4 ) + " mm");

    setVector(3, p1, n1, r, "na: " + toString( n1.length()*1000, 4 ) + " mm");
    setVector(4, p2, n2, g, "nb: " + toString( n2.length()*1000, 4 ) + " mm");
    setVector(5, p3, n3, y, "nc: " + toString( n3.length()*1000, 4 ) + " mm");

    setAngle (6, p1, -v2, v3, y,r, "i: " + toString( a1, 4 ) + " deg");
    setAngle (7, p2, v1, -v3, g,r, "j: " + toString( a2, 4 ) + " deg");
    setAngle (8, p3, -v1, v2, g,y, "k: " + toString( a3, 4 ) + " deg");
}

