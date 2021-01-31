#include "VRLod.h"
#include "geometry/VRGeometry.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRStorage_template.h"
#include "core/objects/object/OSGCore.h"

#include <OpenSG/OSGDistanceLOD.h>
#include <OpenSG/OSGVector.h>

using namespace OSG;

template<> string typeName(const VRLod& t) { return "Lod"; }


VRLod::VRLod(string name) : VRObject(name) {
    lod = DistanceLOD::create();
    setCore(OSGCore::create(lod), "Lod");

    store("center", &center);
    store("scale", &scale);
    store("distances", &distances_string);
    regStorageSetupFkt( VRStorageCb::create("lod setup", bind(&VRLod::loadSetup, this, placeholders::_1)) );
}

VRLod::~VRLod() {}

VRLodPtr VRLod::create(string name) { return shared_ptr<VRLod>(new VRLod(name) ); }
VRLodPtr VRLod::ptr() { return static_pointer_cast<VRLod>( shared_from_this() ); }

void VRLod::setCenter(Vec3d c) { center = c; setup(); }
void VRLod::setScale(double s) { scale = s; setup(); }
void VRLod::setDecimate(bool b, int N) { decimate = b; decimateNumber = N; setup(); }
void VRLod::setDistance(unsigned int i, float dist) { distances[i] = dist; setup(); }
void VRLod::addDistance(float dist) { setDistance(distances.size(), dist); }
Vec3d VRLod::getCenter() { return center; }
double VRLod::getScale() { return scale; }
bool VRLod::getDecimate() { return decimate; }
int VRLod::getDecimateNumber() { return decimateNumber; }

void VRLod::loadSetup(VRStorageContextPtr context) {
    stringstream ss(distances_string);
    float d = 0;
    for (int i=0; ss >> d; i++) distances[i] = d;
    setup();
}

VRObjectPtr VRLod::copy(vector<VRObjectPtr> childs) {
    VRLodPtr _lod = VRLod::create(getName() + "_copy");
    _lod->setCenter(Vec3d(lod->getCenter()));

    MFReal32* vec = lod->editMFRange();
    for (unsigned int i=0;i<vec->size();i++) {
        float tmp = lod->getRange(i);
        _lod->lod->editMFRange()->push_back(tmp);
    }

    return _lod;
}

vector<float> VRLod::getDistances() {
    unsigned int cN = max(getChildrenCount(), (size_t)1)-1;
    vector<float> res(cN);
    for (auto d : distances) {
        if (d.first >= cN) continue;
        res[d.first] = d.second;
    }
    return res;
}

void VRLod::decimateGeometries(VRObjectPtr o, float f) {
    vector<VRObjectPtr> v = o->getObjectListByType("Geometry");
    for (auto o : v) {
        VRGeometryPtr g = static_pointer_cast<VRGeometry>(o);
        string n = g->getName();
        string t = g->getType();
        cout << " dG " << n << " " << t << endl;
        g->decimate(f);
    }
}

void VRLod::setup() {
    stringstream ss;
    ss << distances.size();
    for (auto d : distances) ss << " " << d.second;
    distances_string = ss.str();

    if (decimate) { // use decimated geometries
        VRObjectPtr o = getChild(0);
        decimateNumber = min(decimateNumber, 7u);// max 7 decimation geometries?

        if (o != 0) { // has a child to decimate
            for (unsigned int i=0; i<decimateNumber; i++) {
                if (decimated.count(i) == 0) {
                    decimated[i] = o->duplicate(true);
                }
                decimation[i] = pow(0.4, i+1);
                if (decimation.count(i))
                    decimateGeometries(decimated[i], decimation[i]);
                addChild(decimated[i]);
            }
        }

    } else for(auto c : decimated) subChild(c.second); // remove the decimated geometries

    MFReal32* dists = lod->editMFRange();
    dists->resize(distances.size(), 0);
    for (auto d : distances) (*dists)[d.first] = d.second*scale;

    lod->setCenter(Pnt3f(center));
}

void VRLod::addEmpty() {
    addChild(VRObject::create("lod_empty"));
}

