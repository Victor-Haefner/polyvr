#include "VRSegmentation.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "addons/Engineering/CSG/Octree/Octree.h"
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGGeoProperties.h>
#include <boost/range/adaptor/reversed.hpp>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRSegmentation::VRSegmentation() {}


Vec3f randC() {return Vec3f(0.5+0.5*rand()/RAND_MAX, 0.5+0.5*rand()/RAND_MAX, 0.5+0.5*rand()/RAND_MAX); }

struct accu_bin {
    float weight;
    Vec4f plane;
};

struct Accumulator {
    accu_bin* data;
    int Da, Dr;
    float Ra, Rr, Dp;

    Accumulator(int Da, int Dr) {
        this->Da = Da;
        this->Dr = Dr;
        Ra = Pi/Da;
        Rr = 5.0/Dr;
        Dp = 0.05*Rr;
        data = new accu_bin[size()];
        clear();

        for (int th=0; th < Da; th++) {
            for (int phi=0; phi < Da; phi++) {
                for (int rho=-Dr; rho < Dr; rho++) {
                    (*this)(th, phi, rho).plane = plane(th, phi, rho);
                }
            }
        }
    }

    ~Accumulator() {
        delete[] data;
    }

    void clear() { memset(data, 0, size()*sizeof(accu_bin)); }

    accu_bin& operator()(int i, int j, int k) { return data[i*Da*Dr*2 + j*Dr*2 + k + Dr]; }

    int size() { return 2*Dr*Da*Da; }

    Vec4f plane(float theta, float phi, float rho) {
        float th = Ra*theta - Pi*0.5;
        float ph = Ra*phi;
        float sin_th = sin(th);
        return Vec4f(cos(ph)*sin_th, cos(th), sin(ph)*sin_th, -rho*Rr);
    }

    bool pOnPlane(Pnt3f& p, Vec4f& pl, float& th) { return abs(pl[0]*p[0]+pl[1]*p[1]+pl[2]*p[2]+pl[3]) < th; }
};

struct Accumulator_octree {
    Octree* octree;
    int Da, Dr;
    float Ra, Rr, Dp;

    Accumulator_octree(int Da, int Dr) {
        this->Da = Da;
        this->Dr = Dr;
        Ra = Pi/Da;
        Rr = 5.0/Dr;
        Dp = 0.05*Rr;
        octree = new Octree(Rr);
    }

    ~Accumulator_octree() {
        delete octree;
    }

    void clear() { ; }

    float getWeight(Vec3f p) {
        Octree* t = octree->get(p[0], p[1], p[2]);
        vector<void*> data = t->getData();
        if (data.size() == 0) return 0;
        accu_bin* bin = (accu_bin*)data[0];
        return bin->weight;
    }

    void pushPoint(Vec3f p) {
        float w = 0;
        Octree* t = octree->get(p[0], p[1], p[2]);
        vector<void*> data = t->getData();
        if (data.size() > 0) {
            accu_bin* bin = (accu_bin*)data[0];
            w = bin->weight;
            bin->weight += 0.1;
        }

        t->add(p[0], p[1], p[2], new accu_bin(), w);
    }

    Vec4f plane(float theta, float phi, float rho) {
        float th = Ra*theta - Pi*0.5;
        float ph = Ra*phi;
        float sin_th = sin(th);
        return Vec4f(cos(ph)*sin_th, cos(th), sin(ph)*sin_th, -rho*Rr);
    }

    bool pOnPlane(Pnt3f& p, Vec4f& pl, float& th) { return abs(pl[0]*p[0]+pl[1]*p[1]+pl[2]*p[2]+pl[3]) < th; }
};

void feedAccumulator(VRGeometry* geo_in, Accumulator& accu) {
    GeoVectorPropertyRecPtr positions = geo_in->getMesh()->getPositions();
    //GeoVectorPropertyRecPtr normals = geo_in->getMesh()->getNormals();

    cout << "start plane extraction of " << geo_in->getName() << " with " << positions->size() << " points" << endl;

    Pnt3f p;
    Vec3f n;
    Vec4f pl;
    for (uint i = 0; i<positions->size(); i++) {
        positions->getValue(p, i);
        //normals->getValue(n, i);

        for (int j = 0; j < accu.size(); j++) {
            pl = accu.data[j].plane;
            if ( accu.pOnPlane(p, pl, accu.Dp) ) accu.data[j].weight += 1;//Vec3f(pl).dot(n); // TODO: check the dot product
        }
    }
}

void feedAccumulatorOctree(VRGeometry* geo_in, Accumulator_octree& accu) {
    GeoVectorPropertyRecPtr positions = geo_in->getMesh()->getPositions();
    //GeoVectorPropertyRecPtr normals = geo_in->getMesh()->getNormals();

    cout << "start plane extraction of " << geo_in->getName() << " with " << positions->size() << " points" << endl;

    Pnt3f p;
    Vec3f n;
    Vec4f pl;
    for (uint i = 0; i < positions->size(); i++) {
        positions->getValue(p, i);
        //accu.pushPoint(p);

        /*for (int i = 0; i < accu.size(); i++) {
            pl = accu.data[i].plane;
            if ( accu.pOnPlane(p, pl, accu.Dp) ) accu.data[i].weight += 1;//Vec3f(pl).dot(n); // TODO: check the dot product
        }*/
    }
}

vector<Vec4f> evalAccumulator(Accumulator& accu) {
    map<int, vector<Vec3f> > accu_results; // the planes by importance
    for (int i=0; i < accu.Da; i++) {
        for (int j=0; j < accu.Da; j++) {
            for (int k=-accu.Dr; k < accu.Dr; k++) {
                int a = accu(i, j, k).weight;
                if (accu_results.count(a) == 0) accu_results[a] = vector<Vec3f>();
                accu_results[a].push_back( Vec3f(i,j,k) );
            }
        }
    }

    vector<Vec4f> planes;
    for (auto pvec : boost::adaptors::reverse(accu_results)) {
        if (pvec.first == 0) continue;
        for (auto m : pvec.second) planes.push_back(accu.plane(m[0], m[1], m[2]));
    }

    return planes;
}

vector<VRGeometry*> extractPointsOnPlane(VRGeometry* geo_in, Vec4f plane, Accumulator& accu, float Dp) {
    GeoVectorPropertyRecPtr positions = geo_in->getMesh()->getPositions();

    vector<VRGeometry*> res; // two geometries, the plane and the rest
    for (int i=0; i<2; i++) {
        VRGeometry* geo = new VRGeometry(geo_in->getName() + "_p");
        GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
        geo->setPositions(pos);
        res.push_back(geo);
    }

    Pnt3f p;
    for (uint i = 0; i<positions->size(); i++) {
        positions->getValue(p, i);
        if (accu.pOnPlane(p, plane, Dp)) res[0]->getMesh()->getPositions()->addValue(p);
        else res[1]->getMesh()->getPositions()->addValue(p);
    }

    for (auto geo : res) {
        geo->setType(GL_POINTS);
        GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();
        for (uint i=0; i<geo->getMesh()->getPositions()->size(); i++) inds->addValue(i);
        geo->setIndices(inds);
        geo->setMaterial(new VRMaterial("pmat"));

        //geo->getMaterial()->setDiffuse(Vec3f(pl));
        geo->getMaterial()->setDiffuse(randC());
        geo->getMaterial()->setPointSize(5);
        geo->getMaterial()->setLit(false);
        geo->addAttachment("dynamicaly_generated", 0);
    }

    return res;
}

vector<VRGeometry*> extractPlanes(VRGeometry* geo_in, int N, float delta) {
    srand(5);

    vector<VRGeometry*> res, tmp;
    for (int i=0; i<N; i++) {
        int D = 10 + 2*i;
        D *= delta;
        Accumulator accu( D, D );
        cout << " accumulator size: " << accu.size() << endl;

        feedAccumulator(geo_in, accu);
        vector<Vec4f> planes = evalAccumulator(accu);
        cout << " planes" << planes.size() << endl;
        tmp = extractPointsOnPlane(geo_in, planes[0], accu, 0.02);
        res.push_back(tmp[0]);
        cout << "  push to res " << tmp[0] << " and rest " << tmp[1] <<endl;
        geo_in = tmp[1];
    }

    cout << "return " << res.size() << " planes " << endl;
    return res;
}

VRObject* VRSegmentation::extractPatches(VRGeometry* geo, SEGMENTATION_ALGORITHM algo, float curvature, float curvature_delta, Vec3f normal, Vec3f normal_delta) {
    vector<VRGeometry*> patches = extractPlanes(geo, curvature, curvature_delta);

    VRObject* anchor = new VRObject("segmentation");
    geo->getParent()->addChild(anchor);
	for (auto p : patches) {
        anchor->addChild(p);
        p->setWorldMatrix(geo->getWorldMatrix());
	}

    return anchor;
}

OSG_END_NAMESPACE;
