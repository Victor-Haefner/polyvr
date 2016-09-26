#include "StreetLamp.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRStroke.h"
#include "core/math/path.h"
#include "core/math/pose.h"
#include "core/scene/import/VRImport.h"

#include <OpenSG/OSGMatrixUtility.h>

using namespace OSG;

void StreetLamp::make2(const pose& p, VRGeoData* geo) {
    static int once = 0;
    if (once) return;
    once = 1;

	auto lamp = VRTransform::create("lamp");
	lamp->setPose(Vec3f(0,0,0), Vec3f(0,1,0), Vec3f(0,0,1));

	auto addPart = [&](float r, path& p) {
		int N = 6;
		vector<Vec3f> prof;
		for (int i=0; i<N; i++) {
			float a = i*2*Pi/N;
			float x = r*cos(a);
			float y = r*sin(a);
			prof.push_back(Vec3f(x,y,0));
		}

        auto pole = VRStroke::create("pole");
		pole->addPath(&p);
        pole->strokeProfile(prof, 1, 1);
        lamp->addChild(pole);
	};

	auto p1 = path();
	p1.addPoint(Vec3f(0,0,0), Vec3f(0,0,-1), Vec3f(0,0,0), Vec3f(0,1,0));
	p1.addPoint(Vec3f(0,0,-2), Vec3f(0,0,-1), Vec3f(0,0,0), Vec3f(0,1,0));
	p1.compute(2);

	auto p2 = path();
	p2.addPoint(Vec3f(0,0,-2), Vec3f(0,0,-1), Vec3f(0,0,0), Vec3f(0,1,0));
	p2.addPoint(Vec3f(0,0,-4), Vec3f(0,0,-1), Vec3f(0,0,0), Vec3f(0,1,0));
	p2.compute(2);

	auto p3 = path();
	p3.addPoint(Vec3f(0,0,-4), Vec3f(0,0,-1), Vec3f(0,0,0), Vec3f(0,1,0));
	p3.addPoint(Vec3f(1,0,-5), Vec3f(2,0,-1), Vec3f(0,0,0), Vec3f(0,1,0));
	p3.compute(5);

	addPart(0.1, p1);
	addPart(0.04, p2);
	addPart(0.04, p3);

    VRTransformPtr obj = VRImport::get()->load( "world/assets/lamps/streetlamp1.2.dae", 0, 1);
    obj->setPose(Vec3f(1,0,-5.08), Vec3f(1,0,2.5), Vec3f(0,0,-1));
    obj->setScale(Vec3f(0.07,0.07,0.07));
    lamp->addChild(obj);

	auto g = merge(lamp);
    g->updateNormals();
	Matrix mR;
	MatrixLookAt(mR, Pnt3f(), Pnt3f(0,1,0), Vec3f(1,0,0));
	Matrix m = p.asMatrix();
	m.mult(mR);
	geo->append(g, m);
}

void StreetLamp::make(const pose& p, float h, VRGeoData* geo) {
    auto pushCylinder = [&](VRGeoData& geo, Vec3f pos, Vec3f dir, float r1, float r2, int Nsides, Vec2f tcs) {
        Vec2f tc1(tcs[1], 0);
        Vec2f tc2(tcs[1], 1);
        Vec2f tc3(tcs[0], 1);
        Vec2f tc4(tcs[0], 0);

        Vec3f up(0,1,0);
        if (dir.cross(up).squareLength() < 0.01 ) up = Vec3f(1,0,0);
        Matrix m;
        Vec3f dp(0,0,0);
        MatrixLookAt(m, dp, dir, up);
        for (int i=0; i<Nsides; i++) {
            float a = i*3.14*2/Nsides;
            float b = (i+1)*3.14*2/Nsides;
            Vec3f p1 = Vec3f(cos(a), sin(a), 0);
            Vec3f p2 = Vec3f(cos(b), sin(b), 0);
            m.mult(p1,p1); m.mult(p2,p2);
            Vec3f n = Vec3f(sin((a+b)*0.5),0,cos((a+b)*0.5));
            geo.pushVert(p1*r1+pos, n, tc1);
            geo.pushVert(p1*r2+pos+dir, n, tc2);
            geo.pushVert(p2*r2+pos+dir, n, tc3);
            geo.pushVert(p2*r1+pos, n, tc4);
            geo.pushQuad();
        }
    };

    Vec3f pole = p.pos();
    Vec3f bdir = p.dir();
    Vec3f bpos = pole+Vec3f(0,0.85*h,0);
    Vec3f lpos = bpos+bdir*0.45-Vec3f(0,0.1,0);

    //VRGeoData& tmp = *geo;
    //pushCylinder(tmp, pole, Vec3f(0,h,0), 0.2, 0.15, 8, Vec2f(0.2,0.3)); // pole
    //pushCylinder(tmp, bpos, bdir, 0.12, 0.12, 8, Vec2f(0.2,0.3)); // branch
    //pushCylinder(tmp, lpos, bdir*0.5, 0.15, 0.15, 8, Vec2f(1,0.75)); // branch

    VRGeoData tmp;
    pushCylinder(tmp, pole, Vec3f(0,h,0), 0.2, 0.15, 8, Vec2f(0.2,0.3)); // pole
    pushCylinder(tmp, bpos, bdir, 0.12, 0.12, 8, Vec2f(0.2,0.3)); // branch
    pushCylinder(tmp, lpos, bdir*0.5, 0.15, 0.15, 8, Vec2f(1,0.75)); // branch

    geo->append(tmp);
}

