#include "VRStroke.h"
#include "core/math/path.h"
#include "VRGeometry.h"

#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGMatrixUtility.h>
#include <OpenSG/OSGGeoProperties.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRStroke::VRStroke(string name) : VRObject(name) {
    mode = -1;
    closed = false;
    lit = false;
    geo = 0;
}

void VRStroke::setPath(path* p) {
    paths.clear();
    paths.push_back(p);
}

void VRStroke::addPath(path* p) {
    paths.push_back(p);
}

void VRStroke::setPaths(vector<path*> p) {
    paths = p;
}

vector<path*>& VRStroke::getPaths() { return paths; }

void VRStroke::strokeProfile(vector<Vec3f> profile, bool closed, bool lit) {
    mode = 0;
    this->profile = profile;
    this->closed = closed;
    this->lit = lit;

    GeoUInt8PropertyRecPtr      Type = GeoUInt8Property::create();
    GeoUInt32PropertyRecPtr     Length = GeoUInt32Property::create();
    GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr      Norms = GeoVec3fProperty::create();
    GeoVec3fPropertyRecPtr      Colors = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr     Indices = GeoUInt32Property::create();

    Vec3f z = Vec3f(0,0,1);
    if (profile.size() == 1) Type->addValue(GL_LINES);
    else Type->addValue(GL_QUADS);

    clearChildren();
    for (uint i=0; i<paths.size(); i++) {
        vector<Vec3f> pnts = paths[i]->getPositions();
        vector<Vec3f> directions = paths[i]->getDirections();
        vector<Vec3f> up_vectors = paths[i]->getUpvectors();
        vector<Vec3f> cols = paths[i]->getColors();

        Vec3f _p;
        for (uint j=0; j<pnts.size(); j++) {
            Vec3f p = pnts[j];
            Vec3f n = directions[j];
            Vec3f u = up_vectors[j];
            Vec3f c = cols[j];

            //float ca = n.dot(z);
            Matrix m;
            //MatrixLookAt(m, Vec3f(0,0,0), n, z.cross(n));
            MatrixLookAt(m, Vec3f(0,0,0), n, u);

            // add new profile points && normals
            for (uint k=0; k<profile.size(); k++) {
                Vec3f tmp = profile[k];
                m.mult(tmp, tmp);

                Pos->addValue(p+tmp);
                tmp.normalize();
                Norms->addValue(tmp);
                Colors->addValue(c);
            }

            if (j==0 && profile.size() > 1) continue;

            // add line
            if (profile.size() == 1) {
                int N = Pos->size();
                Indices->addValue(N-2);
                Indices->addValue(N-1);
            } else {
                // add quad
                for (uint k=0; k<profile.size()-1; k++) {
                    int N1 = Pos->size() - 2*profile.size() + k;
                    int N2 = Pos->size() -   profile.size() + k;
                    Indices->addValue(N1);
                    Indices->addValue(N2);
                    Indices->addValue(N2+1);
                    Indices->addValue(N1+1);

                    //cout << "\nN1N2 " << N1 << " " << N2 << " " << N2+1 << " " << N1+1 << flush;
                }

                if (closed) {
                    int N0 = Pos->size() - 2*profile.size();
                    int N1 = Pos->size() - profile.size() - 1;
                    int N2 = Pos->size() - 1;
                    Indices->addValue(N1);
                    Indices->addValue(N2);
                    Indices->addValue(N1+1);
                    Indices->addValue(N0);

                    //cout << "\nN1N2 " << N1 << " " << N2 << " " << N1+1 << " " << N0 << flush;
                }
            }
        }
    }

    Length->addValue(Indices->size());


    SimpleMaterialRecPtr Mat = SimpleMaterial::create();
    GeometryRecPtr g = Geometry::create();
    g->setTypes(Type);
    g->setLengths(Length);
    g->setPositions(Pos);

    g->setNormals(Norms);
    g->setColors(Colors);
    g->setIndices(Indices);

    g->setMaterial(Mat);
    Mat->setLit(lit);

    VRGeometry* geo = new VRGeometry("stroke");
    geo->setMesh(g);
    addChild(geo);
}

void VRStroke::strokeStrew(VRGeometry* geo) {
    if (geo == 0) return;

    mode = 1;
    this->geo = geo;

    clearChildren();
    for (uint i=0; i<paths.size(); i++) {
        vector<Vec3f> pnts = paths[i]->getPositions();
        for (uint j=0; j<pnts.size(); j++) {
            Vec3f p = pnts[j];
            VRGeometry* g = (VRGeometry*)geo->duplicate();
            addChild(g);
            g->translate(p);
        }
    }
}

void VRStroke::update() {
    switch (mode) {
        case 0:
            strokeProfile(profile, closed, lit);
            break;
        case 1:
            strokeStrew(geo);
            break;
        default:
            break;
    }
}

OSG_END_NAMESPACE;
