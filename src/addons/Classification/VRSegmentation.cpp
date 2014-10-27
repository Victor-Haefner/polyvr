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
    float* data;
    int Da, Dr;
    float Ra, Rr, Dp;

    Accumulator() {
        Da = 10;
        Dr = 10;
        Ra = Pi/Da;
        Rr = 5.0/Dr;
        Dp = 0.05*Rr;
        data = new float[size()];
        memset (data, 0, size()*sizeof(float));
    }

    ~Accumulator() {
        delete data;
    }

    float& operator()(int i, int j, int k) { return data[i*Da*Dr*2 + j*Dr*2 + k + Dr]; }

    int size() { return 2*Dr*Da*Da; }
};

Vec3f randC() {return Vec3f(0.5+0.5*rand()/RAND_MAX, 0.5+0.5*rand()/RAND_MAX, 0.5+0.5*rand()/RAND_MAX); }

Vec4f plane(float theta, float phi, float rho, Accumulator& accu) {
    float th = accu.Ra*theta - Pi*0.5;
    float ph = accu.Ra*phi;
    Vec3f n = Vec3f(cos(ph)*sin(th), cos(th), sin(ph)*sin(th));
    return Vec4f(n[0], n[1], n[2], -rho*accu.Rr);
}

bool pOnPlane(Pnt3f& p, Vec4f& pl, Accumulator& accu) { return abs(pl[0]*p[0]+pl[1]*p[1]+pl[2]*p[2]+pl[3]) < accu.Dp; }

vector<VRGeometry*> extractPlanes(VRGeometry* pnts) {

    Accumulator accu;

    GeoVectorPropertyRecPtr positions = pnts->getMesh()->getPositions();
    GeoVectorPropertyRecPtr normals = pnts->getMesh()->getNormals();

    cout << "start plane extraction of " << pnts->getName() << " with " << positions->size() << " points" << endl;
    cout << " accumulator size: " << accu.size() << endl;

    Pnt3f p;
    Vec3f n;
    Vec4f pl;
    for (uint i = 0; i<positions->size(); i++) {
        positions->getValue(p, i);
        normals->getValue(n, i);

        for (int th=0; th < accu.Da; th++) {
            for (int phi=0; phi < accu.Da; phi++) {
                if (th == int(accu.Da*0.5) and phi != 0) continue;
                for (int rho=-accu.Dr; rho < accu.Dr; rho++) { // TODO: segment similar normals and not planes? 3D -> 2D ??
                    pl = plane(th, phi, rho, accu); // TODO: compute all planes before
                    if ( pOnPlane(p, pl, accu) ) {
                        accu(th, phi, rho) += Vec3f(pl).dot(n); // TODO: check the not product with the normal, doesnt seem to work right..
                    }
                }
            }
        }
    }

    cout << " accumulation done " << endl;

    map<int, vector<Vec3f> > accu_results; // the planes by importance
    for (int i=0; i < accu.Da; i++) {
        for (int j=0; j < accu.Da; j++) {
            for (int k=-accu.Dr; k < accu.Dr; k++) {
                int a = accu(i, j, k);
                if (accu_results.count(a) == 0) accu_results[a] = vector<Vec3f>();

                accu_results[a].push_back( Vec3f(i,j,k) );
            }
        }
    }

    vector<Vec4f> planes;
    for (auto pvec : boost::adaptors::reverse(accu_results)) {
        if (pvec.first == 0) continue;
        for (auto m : pvec.second) planes.push_back(plane(m[0], m[1], m[2], accu));
    }

    map<int, VRGeometry*> res;
    for (uint i = 0; i<positions->size(); i++) {
        positions->getValue(p, i);

        int k = 0;
        for (auto pl : planes) {
            if (pOnPlane(p, pl, accu)) {

                if (res.count(k) == 0) {
                    VRGeometry* geo = new VRGeometry(pnts->getName() + "_patch");
                    GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
                    geo->setPositions(pos);
                    res[k] = (geo);
                }

                res[k]->getMesh()->getPositions()->addValue(p);
                break;
            }
            k++;
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

    // TODO: remove all points from the biggest plane and restart the whole process

    return res2;
}

vector<VRGeometry*> VRSegmentation::extractPatches(VRGeometry* geo, SEGMENTATION_ALGORITHM algo, float curvature, float curvature_delta, Vec3f normal, Vec3f normal_delta) {
    vector<VRGeometry*> patches = extractPlanes(geo);

	for (auto p : patches) geo->getParent()->addChild(p);

    return patches;
}

OSG_END_NAMESPACE;
