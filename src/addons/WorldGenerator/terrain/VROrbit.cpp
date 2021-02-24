#include "VROrbit.h"
#include "core/utils/toString.h"
#include <cmath>

const double pi = 3.14159265359;

using namespace OSG;

double VROrbit::toRad(double deg) { return pi*deg/180; }

VROrbit::VROrbit() {}
VROrbit::~VROrbit() {}

VROrbitPtr VROrbit::create() { return VROrbitPtr( new VROrbit() ); }

void VROrbit::setReferential(VRObjectPtr obj) { referential = obj; }
void VROrbit::setTarget(VRObjectPtr obj) { target = obj; }
void VROrbit::setTrail(VRObjectPtr obj) { trail = obj; }
VRObjectPtr VROrbit::getReferential() { return referential; }
VRObjectPtr VROrbit::getTarget() { return target; }
VRObjectPtr VROrbit::getTrail() { return trail; }

void VROrbit::fromKepler(vector<double> p) {
    if (p.size() != 12) {
        cout << "Warning in VROrbit::fromKepler, wrong number of argument, got " << p.size() << " parameters, need 12!" << endl;
        return;
    }
    params.a0 = p[0];
    params.e0 = p[1];
    params.I0 = p[2];
    params.l0 = p[3];
    params.w0 = p[4];
    params.O0 = p[5];
    params.da = p[6];
    params.de = p[7];
    params.dI = p[8];
    params.dl = p[9];
    params.dw = p[10];
    params.dO = p[11];
}

void VROrbit::fromCircle(Vec3d plane, double radius, double speed) {
    params.a0 = radius;
    params.e0 = 0.0;
    params.I0 = plane.enclosedAngle(Vec3d(0,1,0));
    params.l0 = 360.0*float(rand())/RAND_MAX;
    params.w0 = 0.0;
    params.O0 = 0.0;
    params.da = 0.0;
    params.de = 0.0;
    params.dI = 0.0;
    params.dl = speed;
    params.dw = 0.0;
    params.dO = 0.0;
}

double VROrbit::computeEccentricAnomaly(double M, double e) {
    double E0 = M + e*sin(M);
    double En = E0;
    double DE = 1;
    while (abs(DE) > 1e-6) {
        double DM = M - (En-e*sin(En));
        DE = DM/(1.0-e*cos(En));
        En += DE;
    }
    return En;
}

Vec3d VROrbit::computeCoords(double Teph) {
    double T = (Teph-2451545.0)/36525.0; // centuries passed since Jan 2000
    double a = (params.a0 + T*params.da) * 149597870.7; // in km
    double e = params.e0 + T*params.de;
    double I = toRad(params.I0 + T*params.dI);
    double l = toRad(params.l0 + T*params.dl);
    double w = toRad(params.w0 + T*params.dw);
    double o = toRad(params.O0 + T*params.dO);

    double v = w-o;   // argument of perihelion
    double M = fmod(l - w, 2*pi); // mean anomaly
    if (M > pi) M -= 2*pi;

    double E = computeEccentricAnomaly(M, e);

    // coords in orbit plane
    double pX = a*(cos(E)-e);
    double pY = a*sqrt(1-e*e)*sin(E);

    // coords in ecliptic plane
    double X = (cos(v)*cos(o)-sin(v)*sin(o)*cos(I))*pX + (-sin(v)*cos(o)-cos(v)*sin(o)*cos(I))*pY;
    double Y = sin(v)*sin(I)*pX + cos(v)*sin(I)*pY;
    double Z = (cos(v)*sin(o)+sin(v)*cos(o)*cos(I))*pX + (-sin(v)*sin(o)+cos(v)*cos(o)*cos(I))*pY;

    return Vec3d(X,Y,Z);
}

double VROrbit::getPeriod() {
    double M = toRad(params.dl - params.dw);
    return (2*pi/M)*36525.0;
}




