#include "VRSegmentation.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGGeoProperties.h>
#include <boost/range/adaptor/reversed.hpp>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRSegmentation::VRSegmentation() {}

struct Accumulator {
    int* data;
    int Da, Dr;
    float Ra, Rr;

    Accumulator() {
        Da = 10;
        Dr = 10;
        Ra = Pi/Da;
        Rr = 5.0/Dr;
        data = new int[size()];
        memset (data, 0, size()*sizeof(int));
    }

    ~Accumulator() {
        delete data;
    }

    int& operator()(int i, int j, int k) { return data[i*Da*Dr*2 + j*Dr*2 + k + Dr]; }

    int size() { return 2*Dr*Da*Da; }
};

Vec3f randC() {return Vec3f(0.5+0.5*rand()/RAND_MAX, 0.5+0.5*rand()/RAND_MAX, 0.5+0.5*rand()/RAND_MAX); }

Vec4f plane(float theta, float phi, float rho, Accumulator& accu) {
    float th = accu.Ra*theta - Pi*0.5;
    float ph = accu.Ra*phi;
    Vec3f n = Vec3f(cos(ph)*sin(th), cos(th), sin(ph)*sin(th));
    return Vec4f(n[0], n[1], n[2], -rho*accu.Rr);
}

bool pOnPlane(Vec4f p, Vec4f pl, Accumulator& accu) { return abs(pl.dot(p)) < 0.1*max(accu.Ra, accu.Rr); }
bool pOnPlane(Pnt3f p, Vec4f pl, Accumulator& accu) { return pOnPlane(Vec4f(p[0], p[1], p[2], 1), pl, accu); }

vector<VRGeometry*> extractPlanes(VRGeometry* pnts) {

    //cout << "test plane " << plane(0,0,0, 5,5,1) << endl;
    //cout << "test plane " << plane(Pnt3f(12,0,56), plane(0,0,0, 5,5,1) ) << endl;
    //return vector<VRGeometry*>(); // test

    Accumulator accu;

    GeoVectorPropertyRecPtr positions = pnts->getMesh()->getPositions();
    //positions = positions->deepClone();

    cout << "start plane extraction of " << pnts->getName() << " with " << positions->size() << " points" << endl;
    cout << " accumulator size: " << accu.size() << endl;

    Pnt3f p;
    for (uint i = 0; i<positions->size(); i++) {
        positions->getValue(p, i);

        /*Vec4f pl = plane(5, 0, -2, accu);
        bool b = pOnPlane(p, pl, accu);
        if ( b ) accu(5, 0, -2) += 1;
        cout << " pnt " << p << " belongs to " << pl << "? " << b << endl;*/

        for (int th=0; th < accu.Da; th++) {
            for (int phi=0; phi < accu.Da; phi++) {
                if (th == int(accu.Da*0.5) and phi != 0) continue;
                for (int rho=-accu.Dr; rho < accu.Dr; rho++) {
                    Vec4f pl = plane(th, phi, rho, accu);
                    if ( pOnPlane(p, pl, accu) ) accu(th, phi, rho) += 1;
                }
            }
        }
    }

    cout << " accumulation done " << endl;

    map<int, vector<Vec3f> > planes; // the planes by importance
    for (int i=0; i < accu.Da; i++) {
        for (int j=0; j < accu.Da; j++) {
            for (int k=-accu.Dr; k < accu.Dr; k++) {
                int a = accu(i, j, k);
                if (planes.count(a) == 0) planes[a] = vector<Vec3f>();

                planes[a].push_back( Vec3f(i,j,k) );
            }
        }
    }

    for (auto pl : planes) {
        cout << " found " << pl.second.size() << " planes with " << pl.first << " points " << endl;
    }

    map<int, VRGeometry*> res;
    for (uint i = 0; i<positions->size(); i++) {
        positions->getValue(p, i);

        int k=0;
        for (auto pvec : boost::adaptors::reverse(planes)) {
            if (pvec.first == 0) continue;

            for (auto m : pvec.second) {
                if (k++ == 10) break;

                    VRGeometry* geo = new VRGeometry(pnts->getName() + "_patch");
                    res[k] = (geo);
                    GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
                    geo->setPositions(pos);

                Vec4f pl = plane(m[0], m[1], m[2], accu);
                if (pOnPlane(p, pl, accu)) pos->addValue(p);

            }

        }
    }

    cout << " segmentation done" << endl;

    vector<VRGeometry*> res2;
    for (auto r : res) {
        VRGeometry* geo = r.second;

        geo->setType(GL_POINTS);
        GeoUInt32PropertyRefPtr inds = GeoUInt32Property::create();
        for (uint i=0; i<geo->getMesh()->getPositions()->size(); i++) inds->addValue(i);
        geo->setIndices(inds);
        geo->setMaterial(new VRMaterial("pmat"));

        geo->getMaterial()->setDiffuse(randC());
        //geo->getMaterial()->setDiffuse(Vec3f(pl));
        geo->getMaterial()->setPointSize(5);
        geo->getMaterial()->setLit(false);
        geo->addAttachment("dynamicaly_generated", 0);
        res2.push_back(geo);
    }

    return res2;
}

vector<VRGeometry*> VRSegmentation::extractPatches(VRGeometry* geo, SEGMENTATION_ALGORITHM algo, float curvature, float curvature_delta, Vec3f normal, Vec3f normal_delta) {
    vector<VRGeometry*> patches = extractPlanes(geo);

	for (auto p : patches) geo->getParent()->addChild(p);

    return patches;
}

OSG_END_NAMESPACE;
