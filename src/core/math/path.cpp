#include "path.h"
#include "core/objects/VRTransform.h"
#include "core/math/equation.h"
#include "core/utils/VRStorage_template.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

path::path(int d) : degree(d) {
    storeVec("points", points);
    storeVec("point_colors", point_colors);
    store("degree", &degree);
    store("direction", &direction);
    store("iterations", &iterations);
    store("closed", &closed);
}

path::~path() {}

shared_ptr<path> path::create() { return shared_ptr<path>(new path()); }

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

vector<float> path::computeInflectionPoints(int i, int j) { // first and second derivative are parallel
    auto& P1 = points[i];
    auto& P2 = points[j];

    // help points
    float L = (P1.pos() - P2.pos()).length();
    Vec3f H1 = P1.pos() + P1.dir()*0.333*L;
    Vec3f H2 = P2.pos() - P2.dir()*0.333*L;

    // At*t*t + B*t*t + C*t + D
    Vec3f A = P2.pos() - H2*3 + H1*3 - P1.pos();
    Vec3f B = H2 - H1*2 + P1.pos();
    Vec3f C = H1 - P1.pos();

    Vec3f CxB = C.cross(B);
    Vec3f CxA = C.cross(A);
    Vec3f BxA = B.cross(A);

    // CxB + CxA*t + BxA*t*t = 0
    equation ex(0,BxA[0],CxA[0],CxB[0]);
    equation ey(0,BxA[1],CxA[1],CxB[1]);
    equation ez(0,BxA[2],CxA[2],CxB[2]);
    vector<float> T;
    Vec3f t;
    for (int k=0; k < ex.solve(t[0],t[1],t[3]) ;k++) T.push_back(t[k]);
    for (int k=0; k < ey.solve(t[0],t[1],t[3]) ;k++) T.push_back(t[k]);
    for (int k=0; k < ez.solve(t[0],t[1],t[3]) ;k++) T.push_back(t[k]);

    vector<float> R;
    for (auto t : T) {
        Vec3f Vt = (A*t*t+B*t*2+C)*3;
        Vec3f At = (A*t+B)*6;
        if (Vt.dot(At) != 0) continue;
        for (auto r : R) if (r == t) continue;
        R.push_back(t);
    }
    return R;
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

    auto toQuadratic = [&](int j, Vec3f& p1, Vec3f& p4, Vec3f& n1, Vec3f& n4, Vec3f& pm, Vec3f& p2, Vec3f& p3) {
        Vec3f nm;

        auto computePoints = [&](int k) {
            float t = 0.5+0.099*k;
            pm = getPose(t, j, j+1).pos();
            //nm = getPose(t, j, j+1).dir();

            // help points
            float L = (p1-p4).length();
            Vec3f H1 = p1 + n1*0.333*L;
            Vec3f H2 = p4 - n4*0.333*L;

            // At*t*t + B*t*t + C*t + D
            Vec3f A = p4 - H2*3 + H1*3 - p1;
            Vec3f B = H2 - H1*2 + p1;
            Vec3f C = H1 - p1;
            nm = (A*t*t+B*t*2+C)*3;
            nm.normalize();

            //cout << "nm " << getPose(t, j, j+1).dir() << "   " << nm << endl;

            p2 = intersect(p1,n1,pm,nm);
            p3 = intersect(p4,n4,pm,nm);
        };

        /*vector<float> inflPnts = computeInflectionPoints(j,j+1);
        if (inflPnts.size() == 0) inflPnts.push_back(0.5);
        for (float t : inflPnts) {
            computePoints();
        }*/

        computePoints(0);
        for (int k = 1; k<5; k++) { // TODO: replace by computing exact inflection points!
            /*if ((p2-pm).dot(nm) > 0) { // p2 beyond pm
                computePoints(-k);
                continue;
            }
            if ((p2-p1).dot(n1) < 0) { // p2 before p1
                computePoints(-k);
                cout << "Waring, p2 before p1, recompute pm! " << (p2-p1).dot(n1) << " nm " << nm << " pm " << pm << endl;
                continue;
            }
            if ((p3-pm).dot(nm) < 0) { // p3 before pm
                computePoints(k);
                continue;
            }
            if ((p3-p4).dot(n4) > 0) { // p3 beyond p4
                computePoints(k);
                continue;
            }
            break;*/
        }

		//cout << "toQuadratic  p1:" << p1 << "  n1:" << n1 << "  p2:" << p2 << "  pm:" << pm << "  nm:" << nm << endl;
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
			Vec3f p1 = points[j].pos();
			Vec3f p4 = points[j+1].pos();
			Vec3f n1 = points[j].dir(); //n1.normalize();
			Vec3f n4 = points[j+1].dir(); //n4.normalize();
			res.push_back(p1);

			if (isLinear(p1,p4,n1,n4)) {
                Vec3f p2 = (p1+p4)*0.5;
                res.push_back(p2);
			} else {
				Vec3f p2,p3,pm;
				toQuadratic(j,p1,p4,n1,n4,pm,p2,p3);
                res.push_back(p2);
                res.push_back(pm);
                res.push_back(p3);
			}
			if (j == points.size()-2) res.push_back(p4);
		}

		points.clear();
		for (auto p : res) points.push_back( pose(p) );
		update();
    }
}

int path::addPoint( const pose& p, Vec3f c ) {
    points.push_back(p);
    point_colors.push_back(c);
    return points.size() - 1;
}

float path::getLength() {
    float l = 0;
    for (int i=1; i<size(); i++) {
        auto p1 = points[i-1].pos();
        auto p2 = points[i].pos();
        l += (p2-p1).length();
    }
    return l;
}

void path::setPoint(int i, const pose& p, Vec3f c ) {
    if (i < 0 || i >= (int)points.size()) return;
    points[i] = p;
    point_colors[i] = c;
}

vector<pose> path::getPoints() { return points; }
pose& path::getPoint(int i) { return points[i]; }
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
            auto& p1 = points[2*i];
            auto& p2 = points[2*i+1];
            auto& p3 = points[2*i+2];
            quadraticBezier(_pts+(N-1)*i, N, p1.pos(), p2.pos(), p3.pos());
        }
    }

    if (degree == 3) {
        // berechne die hilfspunkte fuer die positionen
        for (unsigned int i=0; i<points.size()-1; i++) {
            auto& p1 = points[i];
            auto& p2 = points[i+1];
            auto& c1 = point_colors[i];
            auto& c2 = point_colors[i+1];
            Vec3f r = p2.pos() - p1.pos();
            float L = r.length();
            Vec3f h1 = p1.pos() + p1.dir()*0.333*L;
            Vec3f h2 = p2.pos() - p2.dir()*0.333*L;

            // berechne die hilfspunkte fuer die directions B'(0.5)
            //  B(t) = (1 - t)^3 * p1 + 3t(1-t)^2 * h1 + 3t^2 (1-t) * h2 + t^3 * p2
            // B'(t) = -3(1-t)^2 * p1 + 3(1-t)^2 *  h1 - 6t(1-t) *    h1 - 3t^2 * h2 + 6t(1-t) * h2 + 3t^2 * p2
            Vec3f n = (r-h1+h2)*0.75;

            // berechne hilfspunkt für up vector
            //Vec3f x = n1.cross(u1)*0.5 + n2.cross(u2)*0.5;
            Vec3f u = (p1.up()+p2.up())*0.5;//x.cross(n);
            u.normalize();

            cubicBezier    (_pts+(N-1)*i, N, p1.pos(), p2.pos(), h1, h2);
            quadraticBezier(_drs+(N-1)*i, N, p1.dir(), n, p2.dir());
            quadraticBezier(_ups+(N-1)*i, N, p1.up(), u, p2.up());
            linearBezier   (_cls+(N-1)*i, N, c1, c2);
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
    points.push_back( points[0] );
    closed = true;
}

bool path::isClosed() { return closed; }

Vec3f path::interp(vector<Vec3f>& vec, float t, int i, int j) {
    if (t <= 0) t = 0; if (t >= 1) t = 1; // clamp t
    if (direction == -1) t = 1-t;

    if (j <= 0) j = vec.size()-1;
    else j *= (iterations-1);
    i *= (iterations-1);

    int N = j-i;
    if (N < 0) return Vec3f();

    float tN = t*N;
    int ti = floor(tN);
    float x = tN-ti;
    if (ti >= N) return vec[i+N];

    //cout << "i j ti x v[ti+i] v[ti+i+1]" << i << " " << j << " " << ti << " " << x << " " << vec[i+ti] << " " << vec[i+ti+1] << endl;
    return (1-x)*vec[i+ti] + x*vec[i+ti+1];
}

Vec3f path::getPosition(float t, int i, int j) { return interp(positions, t, i, j); }
Vec3f path::getColor(float t, int i, int j) { return interp(colors, t, i, j); }

void path::getOrientation(float t, Vec3f& dir, Vec3f& up, int i, int j) {
    dir = interp(directions, t, i, j)*direction;
    up = interp(up_vectors, t, i, j);
}

pose path::getPose(float t, int i, int j) {
    Vec3f d,u; getOrientation(t,d,u,i,j);
    return pose(getPosition(t,i,j), d, u);
}

float path::getClosestPoint(Vec3f p) {
    auto positions = getPositions();
    float dist = 1.0e10;
    float t_min = 0;

    for (int i=1; i<positions.size(); i++){
        Vec3f p1 = positions[i-1];
        Vec3f p2 = positions[i];

        auto d = p2-p1;
        auto L = d.length();
        auto t = -(p1-p).dot(d)/L/L;
        auto ps = p1+d*t;
        if (t<0) { ps = p1; t = 0; }
        if (t>1) { ps = p2; t = 1; }
        float D = (ps-p).length();
        if (dist > D) {
            dist = D;
            t_min = (float(i-1)+t)/(positions.size()-1);
        }
    }
    return t_min;
}

float path::getDistance(Vec3f p) {
    auto positions = getPositions();
    float dist = 1.0e10;

    for (int i=1; i<positions.size(); i++){
        Vec3f p1 = positions[i-1];
        Vec3f p2 = positions[i];

        auto d = p2-p1;
        auto L = d.length();
        auto t = -(p1-p).dot(d)/L/L;
        auto ps = p1+d*t;
        if (t<0) ps = p1;
        if (t>1) ps = p2;
        float D = (ps-p).length();
        if (dist > D) dist = D;
    }
    return dist;
}

void path::clear() {
    points.clear();
    positions.clear();
    directions.clear();
    up_vectors.clear();
    colors.clear();
}


OSG_END_NAMESPACE;
