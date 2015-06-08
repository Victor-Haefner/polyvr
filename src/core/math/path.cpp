#include "path.h"

#include <OpenSG/OSGQuaternion.h>
#include "core/objects/VRTransform.h"

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

void path::linearBezier(Vec3f* container, int N, Vec3f p0, Vec3f p1) {
    if (container == 0) container = new Vec3f[N];

    //berechne schritte
    Vec3f DEL = (p1 - p0)*1./(N-1);

    //fülle den vector
    container[0] = p0;
    container[N-1] = p1;
    for (int i=1;i<N-1;i++) container[i] = container[i-1]+DEL;
}

void path::quadraticBezier(Vec3f* container, int N, Vec3f p0, Vec3f p1, Vec3f n) {
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

void path::cubicBezier(Vec3f* container, int N, Vec3f p0, Vec3f p1, Vec3f h0, Vec3f h1) {
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

path::pnt::pnt(Vec3f p, Vec3f n, Vec3f c, Vec3f u) {
    this->p = p;
    this->n = n;
    this->c = c;
    this->u = u;
}

int path::addPoint(Vec3f p, Vec3f n, Vec3f c, Vec3f u) {
    pnt pn(p,n,c,u);
    points.push_back(pn);
    return points.size() - 1;
}

int path::addPoint(VRTransform* t) {
    OSG::Matrix m = t->getWorldMatrix();
    Vec3f p = Vec3f(m[3]);
    Vec3f d = Vec3f(m[2]);
    Vec3f u = Vec3f(m[1]);
    pnt pn(p, d, Vec3f(1,1,1), u);
    points.push_back(pn);
    return points.size() - 1;
}

float path::getLength() { return (points[points.size()-1].p - points[0].p).length();}

void path::setPoint(int i, Vec3f p, Vec3f n, Vec3f c, Vec3f u) {
    if (i < 0 || i >= (int)points.size()) return;
    points[i].p = p;
    points[i].n = n;
    points[i].c = c;
    points[i].u = u;
}

vector<path::pnt> path::getPoints() { return points; }

path::pnt path::getPoint(int i) { return points[i]; }

void path::compute(int N) {
    if (points.size() <= 1) return;

    iterations = N;
    int tN = N*(points.size()-1);
    positions.assign(tN, Vec3f());
    directions.assign(tN, Vec3f());
    up_vectors.assign(tN, Vec3f());
    colors.assign(tN, Vec3f());
    Vec3f* _pts = &positions[0];
    Vec3f* _drs = &directions[0];
    Vec3f* _ups = &up_vectors[0];
    Vec3f* _cls = &colors[0];

    // berechne die hilfspunkte fuer die positionen
    for (unsigned int i=0; i<points.size()-1; i++) {
        pnt p1 = points[i];
        pnt p2 = points[i+1];
        Vec3f r = p2.p - p1.p;
        float L = r.length();
        Vec3f h1 = p1.p + p1.n*0.333*L;
        Vec3f h2 = p2.p - p2.n*0.333*L;

        // berechne die hilfspunkte fuer die directions B'(0.5)
        //  B(t) = (1 - t)^3 * p1 + 3t(1-t)^2 * h1 + 3t^2 (1-t) * h2 + t^3 * p2
        // B'(t) = -3(1-t)^2 * p1 + 3(1-t)^2 *  h1 - 6t(1-t) *    h1 - 3t^2 * h2 + 6t(1-t) * h2 + 3t^2 * p2
        Vec3f n = (r-h1+h2)*0.75;

        // berechne hilfspunkt für up vector
        //Vec3f x = n1.cross(u1)*0.5 + n2.cross(u2)*0.5;
        Vec3f u = (p1.u+p2.u)*0.5;//x.cross(n);
        u.normalize();

        cubicBezier    (_pts+N*i, N, p1.p, p2.p, h1, h2);
        quadraticBezier(_drs+N*i, N, p1.n, p2.n, n);
        quadraticBezier(_ups+N*i, N, p1.u, p2.u, u);
        linearBezier   (_cls+N*i, N, p1.c, p2.c);
    }
}

vector<Vec3f> path::getPositions() { return positions; }
vector<Vec3f> path::getDirections() { return directions; }
vector<Vec3f> path::getUpvectors() { return up_vectors; }
vector<Vec3f> path::getColors() { return colors; }

void path::invert() { direction *= -1; }
void path::update() { compute(iterations); }

void path::close() {
    if (points.size() <= 1) return;
    points.push_back( getPoint(0) );
    closed = true;
}

bool path::isClosed() { return closed; }

Vec3f path::interp(vector<Vec3f>& vec, float t) {
    if (direction == -1) t = 1-t;
    int N = vec.size() -1;
    if (N == -1) return Vec3f();
    float tN = t*N;
    int ti = floor(tN);
    float x = tN-ti;
    if (ti > N) return vec[N];
    return (1-x)*vec[ti] + x*vec[ti+1];
}

Vec3f path::getPosition(float t) { return interp(positions, t); }
Vec3f path::getColor(float t) { return interp(colors, t); }
void path::getOrientation(float t, Vec3f& dir, Vec3f& up) {
    dir = interp(directions, t)*direction;
    up = interp(up_vectors, t);
}


OSG_END_NAMESPACE;
