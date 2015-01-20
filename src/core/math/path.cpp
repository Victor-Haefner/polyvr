#include "path.h"

#include<OpenSG/OSGQuaternion.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

Vec3f path::projectInPlane(Vec3f v, Vec3f n, bool keep_length) {
    n.normalize();
    float l;
    if (keep_length) l = v.length();
    float k = v.dot(n);
    v -= k*n;
    if (keep_length) v = v*(l/v.length());
    return v;
}

void path::calcBezRowOld(Vec3f* &container, int N, Vec3f p0, Vec3f p1, Vec3f n0, Vec3f n1) {
    if (container == 0) container = new Vec3f[N];

    //schrittweite
    float step = 1./(N-1);

    //berechne die hilfspunkte
    Vec3f h[2];
    Vec3f r = p1-p0;

    Vec3f b[3];
    Vec3f DEL[3];
    if (r != Vec3f(0,0,0)) {
        h[0] = p0+projectInPlane(r*(1./3.),n0,false);
        h[1] = p1+projectInPlane(-r*(1./3.),n1,false);

        //berechne die koeffizienten -> p[0]+b[0]t+b[1]t²+b[2]t³
        b[0] = h[0]*3-p0*3;
        b[1] = p0*3+h[1]*3-h[0]*6;
        b[2] = h[0]*3-p0-h[1]*3+p1;

        //berechne schritte
        DEL[2] = b[2]*6*step*step*step;
        DEL[1] = b[1]*2*step*step + DEL[2];
        DEL[0] = b[0]*step + b[1]*step*step + DEL[2]*(1./6.);
    } else {
        DEL[0] = Vec3f(0,0,0);
        DEL[1] = Vec3f(0,0,0);
        DEL[2] = Vec3f(0,0,0);
    }

    //fülle den vector
    container[0] = p0;
    container[N-1] = p1;
    for (int i=1;i<N-1;i++) {
        container[i] = container[i-1]+DEL[0];
        DEL[0] += DEL[1];
        DEL[1] += DEL[2];
    }
}

void path::linearBezier(Vec3f* &container, int N, Vec3f p0, Vec3f p1) {
    if (container == 0) container = new Vec3f[N];

    //berechne schritte
    Vec3f DEL = (p1 - p0)*1./(N-1);

    //fülle den vector
    container[0] = p0;
    container[N-1] = p1;
    for (int i=1;i<N-1;i++) container[i] = container[i-1]+DEL;
}

void path::quadraticBezier(Vec3f* &container, int N, Vec3f p0, Vec3f p1, Vec3f n) {
    if (container == 0) container = new Vec3f[N];

    //schrittweite
    float step = 1./(N-1);

    //berechne die hilfspunkte
    Vec3f b[3];
    Vec3f DEL[3];

    //berechne die koeffizienten -> p[0]+b[0]t+b[1]t²
    b[0] = n*2 - p0*2;
    b[1] = p0 - n*2 + p1;

    //berechne schritte
    DEL[1] = b[1]*2*step*step;
    DEL[0] = b[0]*step + b[1]*step*step;

    //fülle den vector
    container[0] = p0;
    container[N-1] = p1;
    for (int i=1;i<N-1;i++) {
        container[i] = container[i-1]+DEL[0];
        DEL[0] += DEL[1];
    }
}

void path::cubicBezier(Vec3f* &container, int N, Vec3f p0, Vec3f p1, Vec3f h0, Vec3f h1) {
    if (container == 0) container = new Vec3f[N];

    //schrittweite
    float step = 1./(N-1);

    Vec3f b[3];
    Vec3f DEL[3];

    //berechne die koeffizienten -> p[0]+b[0]t+b[1]t²+b[2]t³
    b[0] = h0*3-p0*3;
    b[1] = p0*3+h1*3-h0*6;
    b[2] = h0*3-p0-h1*3+p1;

    //berechne schritte
    DEL[2] = b[2]*6*step*step*step;
    DEL[1] = b[1]*2*step*step + DEL[2];
    DEL[0] = b[0]*step + b[1]*step*step + DEL[2]*(1./6.);

    //fülle den vector
    container[0] = p0;
    container[N-1] = p1;
    for (int i=1;i<N-1;i++) {
        container[i] = container[i-1]+DEL[0];
        DEL[0] += DEL[1];
        DEL[1] += DEL[2];
    }
}


path::path() {}

void path::setStartPoint(Vec3f p, Vec3f n, Vec3f c, Vec3f u) {
    ep1 = p;
    n1 = n;
    c1 = c;
    u1 = u;
}

void path::setEndPoint(Vec3f p, Vec3f n, Vec3f c, Vec3f u) {
    ep2 = p;
    n2 = n;
    c2 = c;
    u2 = u;
}

void path::compute(int N) {
    points.assign(N, Vec3f());
    directions.assign(N, Vec3f());
    up_vectors.assign(N, Vec3f());
    colors.assign(N, Vec3f());
    Vec3f* _pts = &points[0];
    Vec3f* _drs = &directions[0];
    Vec3f* _ups = &up_vectors[0];
    Vec3f* _cls = &colors[0];

    // berechne die hilfspunkte fuer die positionen
    Vec3f r = ep2-ep1;
    float L = r.length();
    Vec3f h1 = ep1 + n1*0.333*L;
    Vec3f h2 = ep2 - n2*0.333*L;

    // berechne die hilfspunkte fuer die directions B'(0.5)
    Vec3f n = (h1-ep1)*3 + (ep1+h2-h1*2)*3 + (h1*3-h2*3-ep1+ep2)*0.75;
    n.normalize();

    // berechne hilfspunkt für up vector
    Vec3f x = n1.cross(u1)*0.5 + n2.cross(u2)*0.5;
    Vec3f u = x.cross(n);
    u.normalize();

    cubicBezier(_pts, N, ep1, ep2, h1, h2);
    quadraticBezier(_drs, N, n1, n2, n);
    quadraticBezier(_ups, N, u1, u2, u);
    linearBezier(_cls, N, c1, c2);
}

vector<Vec3f> path::getPositions() { return points; }
vector<Vec3f> path::getDirections() { return directions; }
vector<Vec3f> path::getUpvectors() { return up_vectors; }
vector<Vec3f> path::getColors() { return colors; }

void path::getStartPoint(Vec3f& p, Vec3f& n, Vec3f& c) {
    p = ep1;
    n = n1;
    c = c1;
}

void path::getEndPoint(Vec3f& p, Vec3f& n, Vec3f& c) {
    p = ep2;
    n = n2;
    c = c2;
}

void path::update() {
    compute(points.size());
}

Vec3f path::getPosition(float t) {
    if (points.size() == 0) return Vec3f();
    float tN = t*(points.size()-1);
    int ti = floor(tN);
    float x = tN-ti;
    if (ti > points.size()-1) return points[points.size()-1];
    return (1-x)*points[ti] + x*points[ti+1];
}

void path::getOrientation(float t, Vec3f& dir, Vec3f& up) {
    if (directions.size() > 0) {
        int S = directions.size()-1;
        float tN = t*S;
        int ti = floor(tN);
        float x = tN-ti;

        if (ti > S) dir = directions[S];
        else dir = (1-x)*directions[ti] + x*directions[ti+1];
    }

    if (up_vectors.size() > 0) {
        int S = up_vectors.size()-1;
        float tN = t*S;
        int ti = floor(tN);
        float x = tN-ti;

        if (ti > S) up = up_vectors[S];
        else up = (1-x)*up_vectors[ti] + x*up_vectors[ti+1];
    }
}

Vec3f path::getColor(float t) {
    if (colors.size() == 0) return Vec3f();
    float tN = t*(colors.size()-1);
    int ti = floor(tN);
    float x = tN-ti;
    if (ti > colors.size()-1) return colors[colors.size()-1];
    return (1-x)*colors[ti] + x*colors[ti+1];
}

OSG_END_NAMESPACE;
