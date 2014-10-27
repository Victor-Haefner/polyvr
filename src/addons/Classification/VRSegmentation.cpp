#include "VRSegmentation.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGGeoProperties.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRSegmentation::VRSegmentation() {}



Vec4f plane(float theta, float phi, float rho, float Da, float Dr, float rs) {
    float th = Pi*theta/Da - Pi*0.5;
    float ph = Pi*phi/Da;
    Vec3f n = Vec3f(cos(ph)*sin(th), cos(th), sin(ph)*sin(th));
    return Vec4f(n[0], n[1], n[2], -rho*rs);
}

bool pOnPlane(Vec4f p, Vec4f pl) { return abs(pl.dot(p)) < 0.01; }
bool pOnPlane(Pnt3f p, Vec4f pl) { return pOnPlane(Vec4f(p[0], p[1], p[2], 1), pl); }

vector<VRGeometry*> extractPlanes(VRGeometry* pnts) {
    int Da = 2*10;
    int Dr = 2*30;
    float rs = 3.0/Dr;
    int* accu = new int[Dr*Da*Da];

    GeoVectorPropertyRecPtr positions = pnts->getMesh()->getPositions();
    cout << "start plane extraction of " << pnts->getName() << " with " << positions->size() << " points" << endl;
    cout << " accumulator size: " << Dr*Da*Da << endl;

    Pnt3f p;
    for (int i = 0; i<positions->size(); i++) {
        positions->getValue(p, i);
        for (int th=0; th < Da; th++) {
            for (int phi=0; phi < Da; phi++) {
                if (th == int(Da*0.5) and phi != 0) continue;
                for (int rho=0; rho < Dr; rho++) {
                    Vec4f pl = plane(th, phi, rho, Da, Dr, rs);
                    if ( pOnPlane(p, pl) ) accu[th*Da*Dr + phi*Dr + rho] += 1;
                }
            }
        }
    }

    map<int, vector<Vec3f> > ms;
    for (int i=0; i < Da; i++) {
        for (int j=0; j < Da; j++) {
            for (int k=0; k < Dr; k++) {
                int a = accu[i*Da*Dr + j*Dr + k];
                if (ms.count(a) == 0) ms[a] = vector<Vec3f>();
                ms[a].push_back( Vec3f(i,j,k) );
            }
        }
    }

    delete accu;

    vector<VRGeometry*> res;
    for (auto m : ms.rbegin()->second) {
        VRGeometry* geo = new VRGeometry(pnts->getName() + "_patch");
        res.push_back(geo);
        GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
        GeoUInt32PropertyRefPtr inds = GeoUInt32Property::create();

        Vec4f pl = plane(m[0], m[1], m[2], Da, Dr, rs);
        for (int i = 0; i<positions->size(); i++) {
            positions->getValue(p, i);
            if (pOnPlane(p, pl)) pos->addValue(p);
        }

        geo->setPositions(pos);
        geo->setType(GL_POINTS);
        for (int i=0; i<pos->size(); i++) inds->addValue(i);
        geo->setIndices(inds);
    }

    return res;
}

Vec3f randC() {return Vec3f(0.5+0.5*rand()/RAND_MAX, 0.5+0.5*rand()/RAND_MAX, 0.5+0.5*rand()/RAND_MAX); }

vector<VRGeometry*> VRSegmentation::extractPatches(VRGeometry* geo, SEGMENTATION_ALGORITHM algo, float curvature, float curvature_delta, Vec3f normal, Vec3f normal_delta) {
    vector<VRGeometry*> patches = extractPlanes(geo);

	for (auto p : patches) {
        p->getMaterial()->setPointSize(5);
		p->getMaterial()->setLit(false);
		geo->getParent()->addChild(p);
	}

    return patches;
}

OSG_END_NAMESPACE;
