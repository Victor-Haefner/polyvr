#include "VRLod.h"
#include "geometry/VRGeometry.h"
#include "core/utils/toString.h"
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRObjectPtr VRLod::copy(vector<VRObjectPtr> childs) {
    VRLodPtr _lod = VRLod::create(getName() + "_copy");
    _lod->setCenter(Vec3f(lod->getCenter()));

    MFReal32* vec = lod->editMFRange();
    for (uint i=0;i<vec->size();i++) {
        float tmp = lod->getRange(i);
        _lod->lod->editMFRange()->push_back(tmp);
    }

    return _lod;
}

/** initialise **/
VRLod::VRLod(string name) : VRObject(name) {
    lod = DistanceLOD::create();
    setCore(lod, "Lod");

    store("center", &center);
    store("distances", &distances_string);
}

VRLod::~VRLod() {}

VRLodPtr VRLod::create(string name) { return shared_ptr<VRLod>(new VRLod(name) ); }
VRLodPtr VRLod::ptr() { return static_pointer_cast<VRLod>( shared_from_this() ); }

void VRLod::setCenter(Vec3f c) { center = c; update(); }
void VRLod::setDecimate(bool b, int N) { decimate = b; decimateNumber = N; update(); }
void VRLod::setDistance(uint i, float dist) { distances[i] = dist; update(); }
void VRLod::addDistance(float dist) { setDistance(distances.size(), dist); }
Vec3f VRLod::getCenter() { return center; }
bool VRLod::getDecimate() { return decimate; }
int VRLod::getDecimateNumber() { return decimateNumber; }

vector<float> VRLod::getDistances() {
    uint cN = max(getChildrenCount(), (size_t)1)-1;
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

void VRLod::update() {
    stringstream ss;
    ss << distances.size();
    for (auto d : distances) ss << " " << d.second;
    distances_string = ss.str();

    if (decimate) { // use decimated geometries
        VRObjectPtr o = getChild(0);
        decimateNumber = min(decimateNumber, 7u);// max 7 decimation geometries?

        if (o != 0) { // has a child to decimate
            for (uint i=0; i<decimateNumber; i++) {
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
    for (auto d : distances) (*dists)[d.first] = d.second;

    lod->setCenter(Pnt3f(center));
}

void VRLod::loadContent(xmlpp::Element* e) {
    VRObject::loadContent(e);

    if (e->get_attribute("center")) center = toVec3f( e->get_attribute("center")->get_value() );

    distances.clear();
    if (e->get_attribute("distances")) {
        stringstream ss( e->get_attribute("distances")->get_value() );
        uint N;
        float d;

        ss >> N;
        for (uint i=0; i<N; i++) {
            ss >> d;
            distances[i] = d;
        }
    }

    update();
}

void VRLod::addEmpty() {
    addChild(VRObject::create("lod_empty"));
}

OSG_END_NAMESPACE;
