#include "VRGeoData.h"
#include "VRGeometry.h"
#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;

struct VRGeoData::Data {
    GeoUInt32PropertyRecPtr types;
    GeoUInt32PropertyRecPtr lengths;
    GeoUInt32PropertyRecPtr indices;
    GeoPnt3fPropertyRecPtr pos;
    GeoVec3fPropertyRecPtr norms;
    GeoVec3fPropertyRecPtr cols;

    int lastPrim = -1;
};

VRGeoData::VRGeoData() { data = shared_ptr<Data>(new Data()); reset(); }

void VRGeoData::reset() {
    data->types = GeoUInt32Property::create();
    data->lengths = GeoUInt32Property::create();
    data->indices = GeoUInt32Property::create();
    data->pos = GeoPnt3fProperty::create();
    data->norms = GeoVec3fProperty::create();
    data->cols = GeoVec3fProperty::create();
}

bool VRGeoData::valid() {
    if (!data->types->size()) { cout << "Triangulator Error: no types!\n"; return false; }
    if (!data->lengths->size()) { cout << "Triangulator Error: no lengths!\n"; return false; }
    if (!data->pos->size()) { cout << "Triangulator Error: no pos!\n"; return false; }
    return true;
}

int VRGeoData::pushVert(Pnt3f p, Vec3f n) {
    data->pos->addValue(p);
    data->norms->addValue(n);
    return data->pos->size()-1;
}

int VRGeoData::pushVert(Pnt3f p, Vec3f n, Vec3f c) {
    data->cols->addValue(c);
    return pushVert(p,n);
}

void VRGeoData::updateType(int t, int N) {
    if (data->lastPrim == t) {
        int i = data->lengths->size()-1;
        int l = data->lengths->getValue(i);
        data->lengths->setValue(l+N, i);
        return;
    }
    data->types->addValue(t);
    data->lastPrim = t;
    data->lengths->addValue(N);
}

void VRGeoData::pushQuad(int i, int j, int k, int l) {
    data->indices->addValue(i);
    data->indices->addValue(j);
    data->indices->addValue(k);
    data->indices->addValue(l);
    updateType(GL_QUADS, 4);
}

void VRGeoData::pushTri(int i, int j, int k) {
    data->indices->addValue(i);
    data->indices->addValue(j);
    data->indices->addValue(k);
    updateType(GL_TRIANGLES, 3);
}

void VRGeoData::pushPoint(int i) {
    if (i >= 0) i = data->pos->size()-1;
    data->indices->addValue(i);
    updateType(GL_POINTS, 1);
}

void VRGeoData::apply(VRGeometryPtr geo) {
    if (!valid()) cout << "VRGeoData::apply failed: data invalid!" << endl;
    geo->setPositions(data->pos);
    geo->setNormals(data->norms);
    geo->setIndices(data->indices);
    geo->setLengths(data->lengths);
    geo->setTypes(data->types);
    if (data->cols->size() > 0) geo->setColors(data->cols);
}

VRGeometryPtr VRGeoData::asGeometry(string name) {
    auto geo = VRGeometry::create(name);
    apply(geo);
    return geo;
}
