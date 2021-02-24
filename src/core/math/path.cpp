#include "path.h"
#include "core/objects/VRTransform.h"
#include "core/math/polygon.h"
#include "core/math/equation.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"

using namespace OSG;

Path::Path(int d) : degree(d) {
    storeVec("points", points);
    storeVec("point_colors", point_colors);
    store("degree", &degree);
    store("direction", &direction);
    store("iterations", &iterations);
    store("closed", &closed);
}

Path::~Path() {}

PathPtr Path::create() { return PathPtr(new Path()); }

bool Path::isCrossing(PathPtr path) {
    for (unsigned int i = 1; i<positions.size(); i++) {
        auto p1 = positions[i-1];
        auto p2 = positions[i];
        auto t = path->getClosestPoint(p1);
        auto p = path->getPosition(t);
        auto d1 = p - p1;
        auto d2 = p - p2;
        if (d1.dot(d2) < 0) return true;
    }
    return false;
}

Vec3d Path::projectInPlane(Vec3d v, Vec3d n, bool keep_length) {
    n.normalize();
    float l;
    if (keep_length) l = v.length();
    float k = v.dot(n);
    v -= n*k;
    if (keep_length) v = v*(l/v.length());
    return v;
}

void Path::linearBezier(Vec3d* container, int N, Vec3d p0, Vec3d p1) {
    if (container == 0) container = new Vec3d[N];

    //berechne schritte
    Vec3d DEL = (p1 - p0)*1./(N-1);

    //fülle den vector
    container[0] = p0;
    container[N-1] = p1;
    for (int i=1;i<N-1;i++) container[i] = container[i-1]+DEL;
}

void Path::quadraticBezier(Vec3d* container, int N, Vec3d p0, Vec3d p1, Vec3d p2) {
    if (container == 0) container = new Vec3d[N];

    //schrittweite
    float step = 1./(N-1);

    //berechne die hilfspunkte
    Vec3d b[3];
    Vec3d DEL[3];

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

void Path::cubicBezier(Vec3d* container, int N, Vec3d p0, Vec3d p1, Vec3d h0, Vec3d h1) {
    if (container == 0) container = new Vec3d[N];

    //schrittweite
    float step = 1./(N-1);

    Vec3d b[3];
    Vec3d DEL[3];

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

vector<double> Path::computeInflectionPoints(int i, int j, float threshold, float accelerationThreshold, Vec3i axis) { // first and second derivative are parallel
    if (j <= i) j = size()-1;
    vector<double> res;
    for (auto k=i+1; k<=j; k++) {
        auto& P1 = points[k-1];
        auto& P2 = points[k];

        // help points
        double L = (P1.pos() - P2.pos()).length();
        Vec3d H1 = P1.pos() + P1.dir()*L/3.0;
        Vec3d H2 = P2.pos() - P2.dir()*L/3.0;

        // At*t*t + B*t*t + C*t + D
        Vec3d A = P2.pos() - H2*3 + H1*3 - P1.pos();
        Vec3d B = H2 - H1*2 + P1.pos();
        Vec3d C = H1 - P1.pos();

        Vec3d CxB = C.cross(B);
        Vec3d CxA = C.cross(A);
        Vec3d BxA = B.cross(A);

        // CxB + CxA*t + BxA*t*t = 0
        equation ex(0,BxA[0],CxA[0],CxB[0]);
        equation ey(0,BxA[1],CxA[1],CxB[1]);
        equation ez(0,BxA[2],CxA[2],CxB[2]);
        vector<double> T;
        Vec3d t;
        if (axis[0]) for (int k=0; k < ex.solve(t[0],t[1],t[3]) ;k++) T.push_back(t[k]);
        if (axis[1]) for (int k=0; k < ey.solve(t[0],t[1],t[3]) ;k++) T.push_back(t[k]);
        if (axis[2]) for (int k=0; k < ez.solve(t[0],t[1],t[3]) ;k++) T.push_back(t[k]);

        for (auto t : T) {
            if (t <= threshold || t >= 1-threshold) continue;
            if (accelerationThreshold != 0) {
                //Vec3d Vt = A*t*t*3 + B*t*2 + C;
                Vec3d At = A*t*6 + B*2;
                if (At.length() < accelerationThreshold) continue;
            }
            bool b = true;
            for (auto r : res) if (abs(r-t) < threshold) { b = false; break; }
            if (b) res.push_back(t);
        }
    }
    sort(res.begin(), res.end());
    return res;
}

void Path::approximate(int d) {
    auto intersect = [&](Pose& p1, Pose& p2) {
		Vec3d d = p2.pos() - p1.pos();
		Vec3d n3 = p1.dir().cross(p2.dir());
		float N3 = n3.dot(n3);
		if (N3 == 0) N3 = 1.0;
		float s = d.cross(p2.dir()).dot(n3)/N3;
		if (s <= 0) return (p1.pos() + p2.pos())*0.5;
		return p1.pos() + p1.dir()*s;
    };

    /*auto toQuadratic = [&](int j, Pose& p1, Pose& p4, Pose& pm, Pose& p2, Pose& p3) {

    };*/

	auto isLinear = [&](Pose& p1, Pose& p2) {
		if (abs(p1.dir().dot(p2.dir())-1.0) > 1e-5) return false;
		Vec3d d = p2.pos()-p1.pos(); d.normalize();
		if (abs(d.dot(p1.dir())-1.0) > 1e-5) return false;
		return true;
	};

    if (d == 2) {
        vector<Pose> res;

		for (unsigned int j=1; j<points.size(); j++) { // p1,p2,pm,p3,p4
			auto p1 = points[j-1];
			auto p4 = points[j];
			res.push_back( p1 );

			/*if (isLinear(p1,p4)) res.push_back( Pose( (p1.pos()+p4.pos())*0.5, p1.dir(), p1.up() ) );
			else {
                //auto Tvec = computeInflectionPoints(j-1,j,0.01,Vec3i(1,0,1));
                //auto Tvec = computeInflectionPoints(j-1,j,0.01);
                //auto Tvec = computeInflectionPoints(j-1,j,1e-6, 0.01);
                //auto Tvec = computeInflectionPoints(j-1,j,1e-4, 0.1, Vec3i(1,0,1));
                auto Tvec = computeInflectionPoints(j-1,j,1e-6,1e-6);
                if (Tvec.size() == 0) Tvec = {0.5};
                else {
                    cout << " Tvec: ";
                    for (auto t : Tvec) cout << t << " ";
                    cout << endl;
                }

                vector<Pose> poses;
                poses.push_back(p1);
                for (auto t : Tvec) poses.push_back( *getPose(t, j-1, j, false) );
                poses.push_back(p4);

                for (unsigned int i=1; i<poses.size()-1; i++) {
                    auto& pm = poses[i];
                    res.push_back( Pose( intersect(poses[i-1],pm) ) );
                    res.push_back(pm);
                    if (i == poses.size()-2) res.push_back( Pose( intersect(poses[i+1],pm) ) );
                }
			}
			if (j == points.size()-1) res.push_back(p4);*/

			// compute all inflection points and store in poses!
            //auto Tvec = computeInflectionPoints(j-1,j,1e-6,1e-6);
            auto Tvec = computeInflectionPoints(j-1,j,0.1,0.1);
            if (Tvec.size() == 0) Tvec = {0.5};
            vector<Pose> poses;
            poses.push_back(p1);
            for (auto t : Tvec) {
                PosePtr pm = getPose(t, j-1, j, false);
                if (isLinear(p1,*pm)) pm->setDir( p1.dir() );
                Vec3d n = pm->dir();
                n.normalize();
                pm->setDir( n );
                //pm->setDir( p1.dir() );
                poses.push_back( *pm );
            }
            poses.push_back(p4);

            for (unsigned int i=1; i<poses.size()-1; i++) {
                auto& p0 = poses[i-1];
                auto& pm = poses[i];
                if (isLinear(p0,pm)) res.push_back( Pose( (p0.pos()+pm.pos())*0.5, p0.dir(), pm.up() ) );
                else res.push_back( Pose( intersect(p0,pm) ) );
                res.push_back(pm);
                if (i == poses.size()-2) {
                    auto& p2 = poses[i+1];
                    if (isLinear(pm,p2)) res.push_back( Pose( (pm.pos()+p2.pos())*0.5, p2.dir(), pm.up() ) );
                    else res.push_back( Pose( intersect(pm,p2) ) );
                }
            }

			if (j == points.size()-1) res.push_back(p4);
		}

		points.clear();
		for (auto p : res) points.push_back(p);
    }

    degree = d;
    update();
}

int Path::addPoint2( Vec3d p, Vec3d d, Color3f c, Vec3d u ) {
    return addPoint(Pose(p,d,u), c);
}

int Path::addPoint( Pose p, Color3f c ) {
    points.push_back(p);
    point_colors.push_back(c);
    return size() - 1;
}

float Path::getLength(int i, int j) {
    float l = 0;
    if (j <= i) j = size()-1;
    if (degree == 3) {
        for (int k=i; k<j; k++) {
            auto p1 = points[k].pos();
            auto p2 = points[k+1].pos();
            l += (p2-p1).length();
        }
    }
    if (degree == 2) {
        for (int k=i; k<j; k+=2) {
            auto p1 = points[k].pos();
            auto p2 = points[k+2].pos();
            l += (p2-p1).length();
        }
    }
    return l;
}

void Path::setPoint(int i, const Pose& p, Color3f c ) {
    if (i < 0 || i >= size()) return;
    points[i] = p;
    point_colors[i] = c;
}

vector<Pose> Path::getPoints() { return points; }
vector<Vec3d> Path::getControlPoints() { return controlPoints; }
Pose& Path::getPoint(int i) { return points[i]; }
int Path::size() { return points.size(); }

Color3f Path::getPointColor(int i) { return point_colors[i]; }
void Path::setPointColor(int i, Color3f c) { point_colors[i] = c; }

void Path::compute(int N) {
    if (points.size() <= 1) return;
    iterations = N;

    int Nsegs = points.size()-1; // degree 3
    if (degree == 2) Nsegs = (points.size()-1)/2;

    int tN = (N-1)*Nsegs + 1; // (N-1) the number of subdivision points, times number of segments, plus the last point of the path

    positions.assign(tN, Vec3d());
    directions.assign(tN, Vec3d(0,0,-1));
    up_vectors.assign(tN, Vec3d(0,1,0));
    colors.assign(tN, Vec3d());
    Vec3d* _pts = &positions[0];
    Vec3d* _drs = &directions[0];
    Vec3d* _ups = &up_vectors[0];
    Vec3d* _cls = &colors[0];

    controlPoints.clear();

    if (degree == 2) {
        for (int i=0; i<Nsegs; i++) {
            auto& p1 = points[2*i];
            auto& p2 = points[2*i+1];
            auto& p3 = points[2*i+2];
            auto& c1 = point_colors[i];
            auto& c2 = point_colors[i+1];
            quadraticBezier(_pts+(N-1)*i, N, p1.pos(), p2.pos(), p3.pos());
            linearBezier   (_drs+(N-1)*i, N, p1.dir(), p3.dir());
            linearBezier   (_ups+(N-1)*i, N, p1.up() , p3.up());
            linearBezier   (_cls+(N-1)*i, N, Vec3d(c1), Vec3d(c2));
        }
    }

    if (degree == 3) {
        // berechne die hilfspunkte fuer die positionen
        for (int i=0; i<Nsegs; i++) {
            auto& p1 = points[i];
            auto& p2 = points[i+1];
            auto& c1 = point_colors[i];
            auto& c2 = point_colors[i+1];
            Vec3d r = p2.pos() - p1.pos();
            float L = r.length();
            Vec3d h1 = p1.pos() + p1.dir()*L/3.0;
            Vec3d h2 = p2.pos() - p2.dir()*L/3.0;
            controlPoints.push_back(h1);
            controlPoints.push_back(h2);

            // the orientation vectors, dir and up, correspond to the derivative of the cubic bezier
            // they are thus quadratic bezier curves
            // to compute the parameters of the quadratic dir and up curves we first need to derive the cubic bezier:
            // B(t)  =   (1 - t)^3 * p1 + 3 t(1 - t)^2               * h1 + 3   t^2(1 - t)        * h2 +  t^3 * p2
            // B'(t) = -3(1 - t)^2 * p1 + 3[ (1 - t)^2 - 2t(1 - t) ] * h1 + 3[ -t^2 + 2t(1 - t) ] * h2 + 3t^2 * p2
            //       =  3(1 - t)^2 * (h1 - p1) + 6(1 - t)t * (h2 - h1) + 3t^2 * (p2 - h2)
            // now we can compute the parameters of the quadratic bezier:
            // Q(t)  =   (1 - t)^2 * q1        + 2(1 - t)t * q2        +  t^2 * q3
            // => q1 = 3*(h1 - p1), q2 = 3*(h2 - h1), q3 = 3*(p2 - h2)

            // the up vector is orthogonal to dir


            // TODO: use the above for the up vector
            Vec3d dMid = (h2-h1)*3;
            Vec3d uMid = (p1.up() + p2.up())*0.5;
            Vec3d xMid = uMid.cross(dMid);
            uMid = dMid.cross(xMid);
            uMid.normalize();

            cubicBezier    (_pts+(N-1)*i, N, p1.pos(), p2.pos(), h1, h2);
            quadraticBezier(_drs+(N-1)*i, N, (h1-p1.pos())*3, (h2-h1)*3, (p2.pos()-h2)*3);
            computeUpVectors(_ups+(N-1)*i, _drs+(N-1)*i, N, p1.up(), p2.up());
            //quadraticBezier(_ups+(N-1)*i, N, p1.up(), uMid, p2.up());
            linearBezier   (_cls+(N-1)*i, N, Vec3d(c1), Vec3d(c2));
        }
    }
}

void Path::computeUpVectors(Vec3d* container, Vec3d* dirs, int N, Vec3d u0, Vec3d u1) {
    if (container == 0) container = new Vec3d[N];

    //schrittweite
    Vec3d DEL = (u1 - u0)*1./(N-1);
    Vec3d uLin = u0;

    Vec3d uLinProj, uLinPrevProj;

    // TODO: continue optimizing algorithm
    //float A = u0.enclosedAngle(u1);
    //float DELA = A*1./(N-1);

    //fülle den vector
    container[0] = u0;
    container[N-1] = u1;
    for (int i=1;i<N-1;i++) {
        //Quaterniond q(dirs[i], DELA);

        uLin += DEL;
        uLinProj = projectInPlane(uLin,dirs[i],true);
        //uLinPrevProj = projectInPlane(container[i-1],dirs[i],true);

        container[i] = uLinProj;
    }
}

vector<Vec3d> Path::getPositions() { return positions; }
vector<Vec3d> Path::getDirections() { return directions; }
vector<Vec3d> Path::getUpVectors() { return up_vectors; }
vector<Vec3d> Path::getColors() { return colors; }

vector<Pose> Path::getPoses() {
    vector<Pose> res;
    for (unsigned int i=0; i<positions.size(); i++) {
        res.push_back( Pose(positions[i], directions[i], up_vectors[i]) );
    }
    return res;
}

void Path::invert() { direction *= -1; }
void Path::update() { compute(iterations); }

void Path::close() {
    if (points.size() <= 1) return;
    points.push_back( points[0] );
    closed = true;
}

bool Path::isClosed() { return closed; }

Vec3d Path::interp(vector<Vec3d>& vec, float t, int i, int j) {
    if (t <= 0) t = 0;
    if (t >= 1) t = 1; // clamp t
    if (direction == -1) t = 1-t;

    if (j <= 0) j = vec.size()-1;
    else j *= (iterations-1);
    i *= (iterations-1);

    int N = j-i;
    if (N < 0) return Vec3d();

    float tN = t*N;
    int ti = floor(tN);
    float x = tN-ti;
    if (ti >= N) return vec[i+N];

    if (i+ti < 0 || i+ti >= (int)vec.size()) return Vec3d();
    if (i+ti+1 < 0 || i+ti+1 >= (int)vec.size()) return Vec3d();

    //cout << "i j ti x v[ti+i] v[ti+i+1]" << i << " " << j << " " << ti << " " << x << " " << vec[i+ti] << " " << vec[i+ti+1] << endl;
    return vec[i+ti]*(1-x) + vec[i+ti+1]*x;
}

Vec3d Path::getPosition(float t, int i, int j, bool fast) {
    if (fast) return interp(positions, t, i, j);

    if (degree == 2) {
        auto& p1 = points[2*i];
        auto& p2 = points[2*i+1];
        auto& p3 = points[2*i+2];
        return p1.pos()*(1-t)*(1-t) + p2.pos()*2*t*(1-t) + p3.pos()*t*t;
    }

    if (degree == 3) {
        auto& p1 = points[i];
        auto& p2 = points[j];

        Vec3d r = p2.pos() - p1.pos();
        float L = r.length();
        Vec3d h1 = p1.pos() + p1.dir()*L/3.0;
        Vec3d h2 = p2.pos() - p2.dir()*L/3.0;

        return p1.pos()*(1-t)*(1-t)*(1-t) + h1*3*t*(1-t)*(1-t) + h2*3*t*t*(1-t) + p2.pos()*t*t*t;
    }

    return Vec3d();
}

Color3f Path::getColor(float t, int i, int j) { return Vec3f(interp(colors, t, i, j)); }

void Path::getOrientation(float t, Vec3d& dir, Vec3d& up, int i, int j, bool fast) {
    if (fast) {
        dir = interp(directions, t, i, j)*direction;
        up  = interp(up_vectors, t, i, j);
    } else {
        if (degree == 2) { // TODO: stretch t over i-j segment!
            auto& p1 = points[2*i];
            auto& p2 = points[2*i+2];
            dir = p1.dir()*(1-t) + p2.dir()*t;
            up  = p1.up() *(1-t) + p2.up() *t;
        }

        if (degree == 3) { // TODO: stretch t over i-j segment!
            auto& p1 = points[i];
            auto& p2 = points[j];

            Vec3d r = p2.pos() - p1.pos();
            float L = r.length();
            Vec3d n = r*3.0/L-p1.dir()-p2.dir();
            Vec3d u = (p1.up()+p2.up())*0.5;
            u.normalize();

            dir = p1.dir()*(1-t)*(1-t) + n*2*t*(1-t) + p2.dir()*t*t;
            up  = p1.up()*(1-t)*(1-t)  + u*2*t*(1-t) + p2.up()*t*t;
        }
    }
}

PosePtr Path::getPose(float t, int i, int j, bool fast) {
    Vec3d d,u; getOrientation(t,d,u,i,j,fast);
    return Pose::create(getPosition(t,i,j,fast), d, u);
}

void Path::set(PosePtr p1, PosePtr p2, int res) {
    clear();
    addPoint(*p1);
    addPoint(*p2);
    compute(res);
}

float Path::getClosestPoint(Vec3d p) {
    float dist2 = 1.0e20;
    float t_min = 0;

    for (unsigned int i=1; i<positions.size(); i++){
        Vec3d p1 = positions[i-1];
        Vec3d p2 = positions[i];

        auto d = p2-p1;
        auto L2 = d.squareLength();
        auto t = -(p1-p).dot(d)/L2;
        auto ps = p1+d*t;
        if (t<0) { ps = p1; t = 0; }
        if (t>1) { ps = p2; t = 1; }
        float D2 = (ps-p).squareLength();
        if (dist2 > D2) {
            dist2 = D2;
            t_min = (float(i-1)+t)/(positions.size()-1);
        }
    }

    return t_min;
}

float Path::getDistanceToHull(Vec3d p) {
    float dist2 = 1.0e20;

    for (unsigned int i=1; i<points.size(); i++){
        Vec3d p1 = points[i-1].pos();
        Vec3d p2 = points[i].pos();
        auto d = p2-p1;
        auto L2 = d.squareLength();
        auto t = -(p1-p).dot(d)/L2;
        auto ps = p1+d*t;
        if (t<0) ps = p1;
        if (t>1) ps = p2;
        float D2 = (ps-p).squareLength();
        if (dist2 > D2) dist2 = D2;
    }

    return sqrt(dist2);
}

float Path::getDistance(Vec3d p) {
    float dist2 = 1.0e20;

    for (unsigned int i=1; i<positions.size(); i++){
        Vec3d p1 = positions[i-1];
        Vec3d p2 = positions[i];
        auto d = p2-p1;
        auto L2 = d.squareLength();
        auto t = -(p1-p).dot(d)/L2;
        auto ps = p1+d*t;
        if (t<0) ps = p1;
        if (t>1) ps = p2;
        float D2 = (ps-p).squareLength();
        if (dist2 > D2) dist2 = D2;
    }

    return sqrt(dist2);
}

void Path::clear() {
    points.clear();
    positions.clear();
    directions.clear();
    up_vectors.clear();
    colors.clear();
}

void clampSegment(int& i, int& j, int N) {
    if (i < 0 || i >= j) j = 0;
    if (j <= i || j >= N) j = N-1;
}

bool Path::isStraight(int i, int j) {
    clampSegment(i, j, points.size());
    Vec3d p1 = points[i].pos();
    Vec3d d1 = points[i].dir();
    Vec3d p2 = points[j].pos();
    Vec3d d2 = points[j].dir();
    Vec3d d = p2-p1;
    d.normalize();
    d1.normalize();
    d2.normalize();
    return abs(d.dot(d1)) > 0.999 && abs(d.dot(d2)) > 0.999;
}

bool Path::isCurve(int i, int j) { // TODO
    clampSegment(i, j, points.size());
    if (isStraight(i,j)) return false;
    auto iPnts = computeInflectionPoints(i,j);
    if (iPnts.size() == 0) return true;
    return false;
}

bool Path::isSinuous(int i, int j) { // TODO
    clampSegment(i, j, points.size());
    if (isStraight(i,j)) return false;
    auto iPnts = computeInflectionPoints(i,j);
    if (iPnts.size() >= 1) return true;
    return false;
}

void Path::translate(Vec3d t) {
    for (auto& p : points) p.setPos( p.pos()+t );
    compute(iterations);
}



