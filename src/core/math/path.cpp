#include "path.h"
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

void path::quadraticBezier(Vec3f* container, int N, Vec3f p0, Vec3f p1, Vec3f p2) {
    if (container == 0) container = new Vec3f[N];

    //schrittweite
    float step = 1./(N-1);

    //berechne die hilfspunkte
    Vec3f b[3];
    Vec3f DEL[3];

    //berechne die koeffizienten -> p[0]+b[0]t+b[1]t²
    b[0] = p1*2 - p0*2;
    b[1] = p0 - p1*2 + p2;

    //berechne schritte
    DEL[1] = b[1]*2*step*step;
    DEL[0] = b[0]*step + b[1]*step*step;

    //fülle den vector
    container[0] = p0;
    container[N-1] = p2;
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


path::path(int d) { degree = d; }

path::pnt::pnt(Vec3f p, Vec3f n, Vec3f c, Vec3f u) {
    this->p = p;
    this->n = n;
    this->c = c;
    this->u = u;
}

void path::approximate(int d) {
    degree = d;

    auto intersect = [&](Vec3f& p1, Vec3f& n1, Vec3f& p2, Vec3f& n2) {
		Vec3f d = p2-p1;
		Vec3f n3 = n1.cross(n2);
		float N3 = n3.dot(n3);
		if (N3 == 0) N3 = 1.0;
		float s = d.cross(n2).dot(n1.cross(n2))/N3;
		return p1 + n1*s;
    };

    auto toQuadratic = [&](Vec3f& p1, Vec3f& p4, Vec3f& n1, Vec3f& n4, Vec3f& pm, Vec3f& p2, Vec3f& p3) {
		p2 = p1+n1;
		p3 = p4-n4;
		Vec3f nm = p3-p2;
		nm.normalize();
		p2 = intersect(p1,n1,pm,nm);
		p3 = intersect(p4,n4,pm,nm);
    };

	auto isLinear = [&](Vec3f& p1, Vec3f& p2, Vec3f& n1, Vec3f& n2) {
		if (abs(n1.dot(n2)-1.0) > 0.00001) return false;
		Vec3f d = p2-p1; d.normalize();
		if (abs(d.dot(n1)-1.0) > 0.00001) return false;
		return true;
	};

    if (degree == 2) {
        vector<Vec3f> res;

		for (uint j=0; j<points.size()-1; j++) { // p1,p2,pm,p3,p4
			Vec3f p1 = points[j].p;
			Vec3f p4 = points[j+1].p;
			Vec3f n1 = points[j].n; //n1.normalize();
			Vec3f n4 = points[j+1].n; //n4.normalize();
			res.push_back(p1);

			if (isLinear(p1,p4,n1,n4)) {
                Vec3f p2 = (p1+p4)*0.5;
                res.push_back(p2);
			} else {
				Vec3f pm = getPose(0.5, j, j+1).pos();
				Vec3f p2,p3;
				toQuadratic(p1,p4,n1,n4,pm, p2,p3);
                res.push_back(p2);
                res.push_back(pm);
                res.push_back(p3);
			}
			if (j == points.size()-2) res.push_back(p4);
		}

		points.clear();
		for (auto p : res) points.push_back(pnt(p,Vec3f(0,0,-1), Vec3f(0,0,0), Vec3f(0,1,0)));
		update();
    }
}

int path::addPoint(Vec3f p, Vec3f n, Vec3f c, Vec3f u) {
    pnt pn(p,n,c,u);
    points.push_back(pn);
    return points.size() - 1;
}

int path::addPoint(VRTransformPtr t) {
    OSG::Matrix m = t->getWorldMatrix();
    Vec3f p = Vec3f(m[3]);
    Vec3f d = Vec3f(m[2]);
    Vec3f u = Vec3f(m[1]);
    pnt pn(p, d, Vec3f(1,1,1), u);
    points.push_back(pn);
    return points.size() - 1;
}

float path::getLength() {
    float l = 0;
    for (int i=1; i<size(); i++) {
        auto p1 = points[i-1].p;
        auto p2 = points[i].p;
        l += (p2-p1).length();
    }
    return l;
}

void path::setPoint(int i, Vec3f p, Vec3f n, Vec3f c, Vec3f u) {
    if (i < 0 || i >= (int)points.size()) return;
    points[i].p = p;
    points[i].n = n;
    points[i].c = c;
    points[i].u = u;
}

vector<path::pnt> path::getPoints() { return points; }
path::pnt path::getPoint(int i) { return points[i]; }
int path::size() { return points.size(); }

void path::compute(int N) {
    if (points.size() <= 1) return;
    iterations = N;

    int Nsegs = points.size()-1; // degree 3
    if (degree == 2) Nsegs = (points.size()-1)/2;

    int tN = (N-1)*Nsegs + 1; // (N-1) the number of subdivision points, times number of segments, plus the last point of the path

    positions.assign(tN, Vec3f());
    directions.assign(tN, Vec3f(0,0,-1));
    up_vectors.assign(tN, Vec3f(0,1,0));
    colors.assign(tN, Vec3f());
    Vec3f* _pts = &positions[0];
    Vec3f* _drs = &directions[0];
    Vec3f* _ups = &up_vectors[0];
    Vec3f* _cls = &colors[0];

    if (degree == 2) {
        for (unsigned int i=0; i<Nsegs; i++) {
            pnt p1 = points[2*i];
            pnt p2 = points[2*i+1];
            pnt p3 = points[2*i+2];
            quadraticBezier(_pts+(N-1)*i, N, p1.p, p2.p, p3.p);
        }
    }

    if (degree == 3) {
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

            cubicBezier    (_pts+(N-1)*i, N, p1.p, p2.p, h1, h2);
            quadraticBezier(_drs+(N-1)*i, N, p1.n, n, p2.n);
            quadraticBezier(_ups+(N-1)*i, N, p1.u, u, p2.u);
            linearBezier   (_cls+(N-1)*i, N, p1.c, p2.c);
        }
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

Vec3f path::interp(vector<Vec3f>& vec, float t, int i, int j) {
    if (t <= 0) t = 0; if (t >= 1) t = 1; // clamp t
    if (j <= 0) j = vec.size()-1;
    if (direction == -1) t = 1-t;
    int N = j-i;
    if (N == -1) return Vec3f();
    float tN = t*N;
    int ti = floor(tN);
    float x = tN-ti;
    if (ti >= N) return vec[i+N];
    return (1-x)*vec[i+ti] + x*vec[i+ti+1];
}

Vec3f path::getPosition(float t, int i, int j) { return interp(positions, t); }
Vec3f path::getColor(float t, int i, int j) { return interp(colors, t); }

void path::getOrientation(float t, Vec3f& dir, Vec3f& up, int i, int j) {
    dir = interp(directions, t)*direction;
    up = interp(up_vectors, t);
}

pose path::getPose(float t, int i, int j) {
    Vec3f d,u; getOrientation(t,d,u,i,j);
    return pose(getPosition(t,i,j), d, u);
}

void path::clear() {
    points.clear();
    positions.clear();
    directions.clear();
    up_vectors.clear();
    colors.clear();
}


OSG_END_NAMESPACE;
