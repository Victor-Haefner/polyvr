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

struct VRPlane {
    Vec3f data; // theta, phi, rho

    VRPlane(float a, float b, float c) : data(a,b,c) {;}
    VRPlane() {;}

    Vec4f plane() {
        float th = data[0] - Pi*0.5;
        float ph = data[1];
        float sin_th = sin(th);
        return Vec4f(cos(ph)*sin_th, cos(th), sin(ph)*sin_th, -data[2]);
    }

    bool on(const Pnt3f& p, const float& th) {
        return abs(dist(p)) < th;
    }

    float dist(const Pnt3f& p) {
        Vec4f pl = plane();
        return pl[0]*p[0]+pl[1]*p[1]+pl[2]*p[2]+pl[3];
    }
};


Vec3f randC() {return Vec3f(0.5+0.5*rand()/RAND_MAX, 0.5+0.5*rand()/RAND_MAX, 0.5+0.5*rand()/RAND_MAX); }

struct Accumulator {
    struct Bin {
        float weight = 0;
        VRPlane plane;
    };

    virtual void pushPoint(Pnt3f p) = 0;

    void pushGeometry(VRGeometry* geo_in) {
        GeoVectorPropertyRecPtr positions = geo_in->getMesh()->getPositions();
        //GeoVectorPropertyRecPtr normals = geo_in->getMesh()->getNormals();

        cout << "start plane extraction of " << geo_in->getName() << " with " << positions->size() << " points" << endl;

        Pnt3f p;
        Vec3f n;
        VRPlane pl;
        for (uint i = 0; i<positions->size(); i++) {
            positions->getValue(p, i);
            //normals->getValue(n, i);
            pushPoint(p);
        }
    }

    virtual vector<VRPlane> eval() = 0;
};

struct Accumulator_grid : public Accumulator {
    Bin* data;
    int Da, Dr;
    float Ra, Rr, Dp;

    Accumulator_grid(int Da, int Dr) {
        this->Da = Da;
        this->Dr = Dr;
        Ra = Pi/Da;
        Rr = 5.0/Dr;
        Dp = 0.05*Rr;
        data = new Bin[size()];
        clear();

        for (int th=0; th < Da; th++) {
            for (int phi=0; phi < Da; phi++) {
                for (int rho=-Dr; rho < Dr; rho++) {
                    (*this)(th, phi, rho).plane = VRPlane(th*Ra, phi*Ra, rho*Rr);
                }
            }
        }
    }

    ~Accumulator_grid() {
        delete[] data;
    }

    void clear() { memset(data, 0, size()*sizeof(Bin)); }

    void pushPoint(Pnt3f p) {
        /*for (int j = 0; j < size(); j++) {
            if ( data[j].plane.on(p, Dp) ) data[j].weight += 1;//Vec3f(pl).dot(n); // TODO: check the dot product
        }*/

        for (int th=0; th < Da; th++) {
            for (int ph=0; ph < Da; ph++) {
                VRPlane p0(th*Ra, ph*Ra, 0);
                float rf = p0.dist(p);
                int r = round(rf/Rr);
                (*this)(th, ph, r).weight += 1;
                //if ( (*this)(th, ph, r).plane.on(p, Dp) ) (*this)(th, ph, r).weight += 1;
            }
        }
    }

    Bin& operator()(int i, int j, int k) { return data[i*Da*Dr*2 + j*Dr*2 + k + Dr]; }

    int size() { return 2*Dr*Da*Da; }

    vector<VRPlane> eval() {
        map<int, vector<VRPlane> > res; // the planes by importance
        for (int i=0; i < Da; i++) {
            for (int j=0; j < Da; j++) {
                for (int k=-Dr; k < Dr; k++) {
                    int a = (*this)(i, j, k).weight;
                    if (res.count(a) == 0) res[a] = vector<VRPlane>();
                    res[a].push_back( (*this)(i, j, k).plane );
                }
            }
        }

        vector<VRPlane> planes;
        for (auto pvec : boost::adaptors::reverse(res)) {
            if (pvec.first == 0) continue;
            for (auto m : pvec.second) planes.push_back(m);
        }

        return planes;
    }
};

struct Accumulator_octree : public Accumulator {
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

    float getWeight(Vec3f p) {
        Octree* t = octree->get(p[0], p[1], p[2]);
        vector<void*> data = t->getData();
        if (data.size() == 0) return 0;
        Bin* bin = (Bin*)data[0];
        return bin->weight;
    }

    void pushPoint(Pnt3f p) {
        float w = 0;
        Octree* t = octree->get(p[0], p[1], p[2]);
        vector<void*> data = t->getData();
        if (data.size() > 0) {
            Bin* bin = (Bin*)data[0];
            w = bin->weight;
            bin->weight += 0.1;
        }

        t->add(p[0], p[1], p[2], new Bin(), w);
    }

    vector<VRPlane> eval() {
        ;
    }
};

vector<VRGeometry*> extractPointsOnPlane(VRGeometry* geo_in, VRPlane plane, float Dp) {
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
        if (plane.on(p, Dp)) res[0]->getMesh()->getPositions()->addValue(p);
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

    /*Accumulator_grid accu( 10*delta, 10*delta );
    accu.pushGeometry(geo_in);
    vector<VRPlane> planes = accu.eval();
    cout << "found " << planes.size() << " planes" << endl;
    for (int i=0; i<N; i++) {
        tmp = extractPointsOnPlane(geo_in, planes[i], 0.02);
        res.push_back( tmp[0] );
    }*/


    for (int i=0; i<N; i++) {
        int D = 10 + 2*i;
        D *= delta;
        Accumulator_grid accu( D, D );
        cout << " accumulator size: " << accu.size() << endl;

        accu.pushGeometry(geo_in);

        vector<VRPlane> planes = accu.eval();
        cout << " planes" << planes.size() << endl;
        tmp = extractPointsOnPlane(geo_in, planes[0], 0.02);
        res.push_back(tmp[0]);
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
