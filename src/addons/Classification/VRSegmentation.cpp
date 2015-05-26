#include "VRSegmentation.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/math/Octree.h"
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGTriangleIterator.h>
#include <boost/range/adaptor/reversed.hpp>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRSegmentation::VRSegmentation() {}

struct VRPlane {
    Vec3f data; // theta, phi, rho

    VRPlane(Pnt3f p, Vec3f n) {
        data[0] = acos(n[1]);
        float csT = max(sin(data[0])+1, cos(data[0])+1)-1;
        data[1] = acos(n[0]/csT);
        data[2] = dist(p);
        //cout << " set plane " << data << "  p " << p << "  n " << n << "  pl " << plane() << " " << dist(p) << endl;
    }

    VRPlane(float a, float b, float c) : data(a,b,c) {;}
    VRPlane() {;}

    Vec4f plane() {
        float th = data[0];
        float ph = data[1];
        float sin_th = sin(th);
        return Vec4f(cos(ph)*sin_th, cos(th), -sin(ph)*sin_th, -data[2]);
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

    virtual void pushPoint(Pnt3f p, Vec3f n) = 0;

    void pushGeometry(VRGeometry* geo_in) {
        GeoVectorPropertyRecPtr positions = geo_in->getMesh()->getPositions();
        GeoVectorPropertyRecPtr normals = geo_in->getMesh()->getNormals();

        cout << "start plane extraction of " << geo_in->getName() << " with " << positions->size() << " points && " << normals->size() << " normals" << endl;

        Pnt3f p;
        Vec3f n;
        VRPlane pl;
        for (uint i = 0; i<positions->size(); i++) {
            positions->getValue(p, i);
            normals->getValue(n, i);
            pushPoint(p,n);
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

        for (int th=0; th <= Da; th++) {
            for (int phi=0; phi <= Da; phi++) {
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

    void pushPoint(Pnt3f p, Vec3f n) {
        VRPlane p0(p, n);
        int T,P,R;
        T = round(p0.data[0]/Ra);
        P = round(p0.data[1]/Ra);
        R = round(p0.data[2]/Rr);
        if (T < 0 || T > Da) cout << " Err T " << Da << " " << T << endl;
        if (P < 0 || P > Da) cout << " Err P " << Da << " " << P << endl;
        if (R < -Dr || R >= Dr) cout << " Err R " << Dr << " " << R << endl;
        Bin& b = (*this)(T, P, R);

        b.weight += 1;
        //b.weight += 1.0/(1.0+b.plane.dist(p));
        //if ( b.plane.on(p, Dp) ) b.weight += 1;
    }

    Bin& operator()(int i, int j, int k) { return data[i*(Da+1)*Dr*2 + j*Dr*2 + k + Dr]; }

    int size() { return 2*Dr*(Da+1)*(Da+1); }

    vector<VRPlane> eval() { // get planes by highest weight
        map<int, vector<VRPlane> > res; // the planes by importance
        for (int i=0; i <= Da; i++) {
            for (int j=0; j <= Da; j++) {
                for (int k=-Dr; k < Dr; k++) {
                    Bin& b = (*this)(i, j, k);
                    int w = b.weight;
                    if (res.count(w) == 0) res[w] = vector<VRPlane>();
                    res[w].push_back( b.plane );
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

    void pushPoint(Pnt3f p, Vec3f n) {
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
        return vector<VRPlane>();
    }
};

vector<VRGeometry*> extractPointsOnPlane(VRGeometry* geo_in, VRPlane plane, float Dp) {
    GeoVectorPropertyRecPtr positions = geo_in->getMesh()->getPositions();

    vector<VRGeometry*> res; // two geometries, the plane && the rest
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

    return res;
}

vector<VRGeometry*> clusterByPlanes(VRGeometry* geo_in, vector<VRPlane> planes, float Dp, int pMax) {
    GeoVectorPropertyRecPtr positions = geo_in->getMesh()->getPositions();
    map<int, VRGeometry*> res_map;
    Pnt3f p;
    int N = min(pMax+1, (int)planes.size());

    for (uint i = 0; i<positions->size(); i++) {
        positions->getValue(p, i);
        int pl = 0;
        float dmin = 1e7;
        float d;
        for (int j=0; j<N; j++) { // go through planes
            d = abs(planes[j].dist(p));
            if (d < dmin) {
                dmin = d;
                pl = j;
            }

            if (dmin < Dp) break;
        }

        if (dmin > Dp) continue;

        if (res_map.count(pl) == 0) { // init geometries for each plane
            VRGeometry* geo = new VRGeometry(geo_in->getName() + "_p");
            GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
            geo->setPositions(pos);
            res_map[pl] = geo;
        }

        res_map[pl]->getMesh()->getPositions()->addValue(p);
    }

    vector<VRGeometry*> res;
    for (auto r : res_map) res.push_back(r.second);
    return res;
}

void finalizeClusterGeometries(vector<VRGeometry*>& res) {
    for (auto geo : res) { // finalize geometries
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
}

void fixNormalsIndices(VRGeometry* geo_in) {
    map<int, int> inds_map;

    GeoIntegralPropertyRecPtr i_n = geo_in->getMesh()->getIndex(Geometry::PositionsIndex);
    GeoIntegralPropertyRecPtr i_p = geo_in->getMesh()->getIndex(Geometry::NormalsIndex);

    GeoVectorPropertyRecPtr norms = geo_in->getMesh()->getNormals();
    GeoVec3fPropertyRecPtr norms2 = GeoVec3fProperty::create();

    for (uint i=0; i<i_p->size(); i++) inds_map[i_n->getValue(i)] = i_p->getValue(i);
    for (uint i=0; i<norms->size(); i++) {
        Vec3f n;
        norms->getValue(n, inds_map[i]);
        norms2->addValue(n);
    }

    geo_in->setNormals(norms2);
    geo_in->getMesh()->setIndex(geo_in->getMesh()->getIndex(Geometry::PositionsIndex), Geometry::NormalsIndex);

    /*GeoColor4fPropertyRecPtr cols = GeoColor4fProperty::create();
    for (uint i = 0; i<norms->size(); i++) {
        Vec3f v; norms->getValue(v, i);
        Vec4f c(abs(v[0]), abs(v[1]), abs(v[2]), 1);
        cols->addValue(c);
    }
    geo_in->getMaterial()->setLit(false);
    geo_in->getMesh()->setIndex(geo_in->getMesh()->getIndex(Geometry::PositionsIndex), Geometry::ColorsIndex);
    geo_in->setColors(cols);*/
    //geo_in->getMesh()->setIndex(geo_in->getMesh()->getIndex(Geometry::PositionsIndex), Geometry::NormalsIndex);
}

vector<VRGeometry*> extractPlanes(VRGeometry* geo_in, int N, float delta) {
    srand(time(0));

    fixNormalsIndices(geo_in);

    Accumulator_grid accu( delta, delta ); cout << "gen accu grid with size " << accu.size() << endl;
    accu.pushGeometry(geo_in);
    vector<VRPlane> planes = accu.eval();  cout << "found " << planes.size() << " planes" << endl;
    //vector<VRGeometry*> res = extractPointsOnPlane(geo_in, planes[0], 0.02);
    vector<VRGeometry*> res = clusterByPlanes(geo_in, planes, 0.005, N);
    finalizeClusterGeometries(res);

    cout << "return " << res.size() << " planes " << endl;
    return res;
}

VRObject* VRSegmentation::extractPatches(VRGeometry* geo, SEGMENTATION_ALGORITHM algo, float curvature, float curvature_delta, Vec3f normal, Vec3f normal_delta) {
    vector<VRGeometry*> patches = extractPlanes(geo, curvature, curvature_delta);

    VRObject* anchor = new VRObject("segmentation");
    anchor->addAttachment("dynamicaly_generated", 0);
    geo->getParent()->addChild(anchor);
	for (auto p : patches) {
        anchor->addChild(p);
        p->setWorldMatrix(geo->getWorldMatrix());
	}

    return anchor;
}

void VRSegmentation::removeDuplicates(VRGeometry* geo) {
    if (geo == 0) return;
    if (geo->getMesh() == 0) return;

    GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr norms = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();
    GeoUInt32PropertyRecPtr lengths = GeoUInt32Property::create();
	size_t curIndex = 0;
	float threshold = 1e-4;
	size_t NLM = numeric_limits<size_t>::max();
	Octree oct(threshold);

	TriangleIterator it(geo->getMesh());
	for (; !it.isAtEnd() ;++it) {
        vector<size_t> IDs(3);
        for (int i=0; i<3; i++) {
            Pnt3f p = it.getPosition(i);
            vector<void*> resultData = oct.radiusSearch(p.x(), p.y(), p.z(), threshold);
            if (resultData.size() > 0) IDs[i] = *(size_t*)resultData.at(0);
            else IDs[i] = NLM;
        }

		for (int i=0; i<3; i++) {
			if (IDs[i] == NLM) {
                Pnt3f p = it.getPosition(i);
                Vec3f n = it.getNormal(i);
				pos->addValue(p);
				norms->addValue(n);
				IDs[i] = curIndex;
				size_t *curIndexPtr = new size_t;
				*curIndexPtr = curIndex;
				oct.add(p.x(), p.y(), p.z(), curIndexPtr);
				curIndex++;
			}
		}

		if (IDs[0] == IDs[1] || IDs[0] == IDs[2] || IDs[1] == IDs[2]) continue;

		for (int i=0; i<3; i++) inds->addValue(IDs[i]);
	}

    lengths->addValue(inds->size());
	geo->setPositions(pos);
	geo->setNormals(norms);
	geo->setIndices(inds);
	geo->setType(GL_TRIANGLES);
	geo->setLengths(lengths);

	for (void* o : oct.getData()) delete (size_t*)o; // Cleanup
}

//#include "addons/Engineering/CSG/CGALTypedefs.h"
//#include "addons/Engineering/CSG/PolyhedronBuilder.h"

struct Vertex;
struct Edge;
struct Triangle;

struct Vertex {
    vector<Edge*> edges;
    vector<Triangle*> triangles;
    Vec3f v;
    Vertex(Pnt3f p) : v(p) {;}
};

struct Edge {
    vector<Vertex*> vertices;
    vector<Triangle*> triangles;
    Edge() {
        vertices = vector<Vertex*>(2,0);
    }
};

struct Triangle {
    vector<Vertex*> vertices;
    vector<Edge*> edges;
    Triangle() {
        edges = vector<Edge*>(3,0);
        vertices = vector<Vertex*>(3,0);
    }
};

void VRSegmentation::fillHoles(VRGeometry* geo) {
    if (geo == 0) return;
    if (geo->getMesh() == 0) return;
    // TODO: check if type is GL_TRIANGLES and just one length!

    vector<Triangle*> triangles;
    map<int, Vertex*> vertices;
    map<int, Edge*> edges;

    /*auto inds = geo->getMesh()->getIndices();
    for (auto i=0; i<inds->size(); i+=3) {
        ;
    }*/

	TriangleIterator it(geo->getMesh());
	for (; !it.isAtEnd() ;++it) {
        Triangle* t = new Triangle();

        for (int i=0; i<3; i++) {
            Pnt3f p = it.getPosition(i);
            Vertex* v = 0;
            int ID = it.getIndex(i);
            if (vertices.count(ID)) v = vertices[ID];
            if (v == 0) v = new Vertex(p);
            vertices[ID] = v;
            t->vertices[i] = v;
            v->triangles.push_back(t);
        }

        for (int i=0; i<3; i++) {
            int ID1 = it.getIndex(i);
            int ID2 = it.getIndex((i+1)%3);
            int ID = (ID1+ID2)*(ID1+ID2+1)*0.5+ID2; // hash IDs edge map
            Edge* e = 0;
            if (edges.count(ID)) e = edges[ID];
            if (e == 0) {
                e = new Edge();
                e->vertices[0] = t->vertices[i];
                e->vertices[1] = t->vertices[(i+1)%3];
                t->vertices[i]->edges.push_back(e);
                t->vertices[(i+1)%3]->edges.push_back(e);
            }
            edges[ID] = e;
            t->edges[i] = e;
            e->triangles.push_back(t);
        }
	}
}

VRObject* VRSegmentation::convexDecompose(VRGeometry* geo) {
    return 0;
}

OSG_END_NAMESPACE;
