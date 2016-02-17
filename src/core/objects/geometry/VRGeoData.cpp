#include "VRGeoData.h"
#include "VRGeometry.h"
#include "core/utils/toString.h"
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>

using namespace OSG;

struct VRGeoData::Data {
    GeoUInt8PropertyRecPtr types;
    GeoUInt32PropertyRecPtr lengths;
    GeoUInt32PropertyRecPtr indices;
    GeoPnt3fPropertyRecPtr pos;
    GeoVec3fPropertyRecPtr norms;
    GeoVec3fPropertyRecPtr cols3;
    GeoVec4fPropertyRecPtr cols4;
    GeoVec2fPropertyRecPtr texs;

    int lastPrim = -1;

    int getColorChannels(GeoVectorProperty* v) {
        if (v == 0) return 0;
        int type = v->getType().getId();
        if (type == 1749) return 3;
        if (type == 1775) return 3;
        if (type == 1776) return 4;
        cout << "getColorChannels WARNING: unknown type ID " << type << endl;
        return 0;
    }
};

VRGeoData::VRGeoData() : pend(this, 0) { data = shared_ptr<Data>(new Data()); reset(); }

VRGeoData::VRGeoData(VRGeometryPtr geo) : pend(this, 0) {
    data = shared_ptr<Data>(new Data());
    data->types = (GeoUInt8Property*)geo->getMesh()->getTypes();
    data->lengths = (GeoUInt32Property*)geo->getMesh()->getLengths();
    data->indices = (GeoUInt32Property*)geo->getMesh()->getIndices();
    data->pos = (GeoPnt3fProperty*)geo->getMesh()->getPositions();
    data->norms = (GeoVec3fProperty*)geo->getMesh()->getNormals();
    data->texs = (GeoVec2fProperty*)geo->getMesh()->getTexCoords();
    auto cols = geo->getMesh()->getColors();
    int Nc = data->getColorChannels(cols);
    if (Nc == 3) data->cols3 = (GeoVec3fProperty*)cols;
    if (Nc == 4) data->cols4 = (GeoVec4fProperty*)cols;
}

void VRGeoData::reset() {
    data->types = GeoUInt8Property::create();
    data->lengths = GeoUInt32Property::create();
    data->indices = GeoUInt32Property::create();
    data->pos = GeoPnt3fProperty::create();
    data->norms = GeoVec3fProperty::create();
    data->cols3 = GeoVec3fProperty::create();
    data->cols4 = GeoVec4fProperty::create();
    data->texs = GeoVec2fProperty::create();
}

bool VRGeoData::valid() {
    if (!data->types->size()) { cout << "Triangulator Error: no types!\n"; return false; }
    if (!data->lengths->size()) { cout << "Triangulator Error: no lengths!\n"; return false; }
    if (!data->pos->size()) { cout << "Triangulator Error: no pos!\n"; return false; }
    return true;
}

int VRGeoData::size() { return data->pos->size(); }

int VRGeoData::pushVert(Pnt3f p) { data->pos->addValue(p); return data->pos->size()-1; }
int VRGeoData::pushVert(Pnt3f p, Vec3f n) { data->norms->addValue(n); return pushVert(p); }
int VRGeoData::pushVert(Pnt3f p, Vec3f n, Vec3f c) { data->cols3->addValue(c); return pushVert(p,n); }
int VRGeoData::pushVert(Pnt3f p, Vec3f n, Vec4f c) { data->cols4->addValue(c); return pushVert(p,n); }
int VRGeoData::pushVert(Pnt3f p, Vec3f n, Vec2f t) { data->texs->addValue(t); return pushVert(p,n); }
int VRGeoData::pushVert(Pnt3f p, Vec3f n, Vec3f c, Vec2f t) { data->texs->addValue(t); return pushVert(p,n,c); }
int VRGeoData::pushVert(Pnt3f p, Vec3f n, Vec4f c, Vec2f t) { data->texs->addValue(t); return pushVert(p,n,c); }

int VRGeoData::pushVert(VRGeoData& other, int i) {
    auto od = other.data;
    Pnt3f p = od->pos->getValue(i);
    bool doNorms = (od->norms && od->norms->size() > i);
    bool doCol3 = (od->cols3 && od->cols3->size() > i);
    bool doCol4 = (od->cols4 && od->cols4->size() > i);
    bool doTex = (od->texs && od->texs->size() > i);
    if (doNorms) {
        Vec3f n = od->norms->getValue(i);
        if (doTex && doCol3) return pushVert(p, n, od->cols3->getValue(i), od->texs->getValue(i));
        if (doTex && doCol4) return pushVert(p, n, od->cols4->getValue(i), od->texs->getValue(i));
        if (doTex)  return pushVert(p, n, od->texs->getValue(i));
        if (doCol3) return pushVert(p, n, od->cols3->getValue(i));
        if (doCol4) return pushVert(p, n, od->cols4->getValue(i));
        return pushVert(p, n);
    }
    return pushVert(p);
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
    if (i < 0) i = data->pos->size()-1;
    data->indices->addValue(i);
    updateType(GL_POINTS, 1);
}

void VRGeoData::pushPrim(Primitive p) {
    int N = p.indices.size();
    for (int i : p.indices) data->indices->addValue(i);
    updateType(p.type, N);
}

void VRGeoData::apply(VRGeometryPtr geo) {
    if (!valid()) cout << "VRGeoData::apply failed: data invalid!" << endl;
    geo->setPositions(data->pos);
    if (data->lengths->size() > 0) geo->setLengths(data->lengths);
    if (data->types->size() > 0) geo->setTypes(data->types);
    if (data->norms->size() > 0) geo->setNormals(data->norms);
    if (data->indices->size() > 0) geo->setIndices(data->indices);
    if (data->cols3->size() > 0) geo->setColors(data->cols3);
    if (data->cols4->size() > 0) geo->setColors(data->cols4);
    if (data->texs->size() > 0) geo->setTexCoords(data->texs);
}

VRGeometryPtr VRGeoData::asGeometry(string name) {
    auto geo = VRGeometry::create(name);
    apply(geo);
    return geo;
}

string VRGeoData::status() {
    string res;
    res += "VRGeoData stats:\n";
    res += " " + toString(data->types->size()) + " types: ";
    for (int i=0; i<data->types->size(); i++) res += " " + toString(data->types->getValue(i));
    res += "\n";
    res += " " + toString(data->lengths->size()) + " lengths: ";
    for (int i=0; i<data->lengths->size(); i++) res += " " + toString(data->lengths->getValue(i));
    res += "\n";
    res += " " + toString(data->pos->size()) + " positions\n";
    res += " " + toString(data->norms->size()) + " normals\n";
    res += " " + toString(data->indices->size()) + " indices\n";
    res += " " + toString(data->cols3->size()) + " colors 3\n";
    res += " " + toString(data->cols4->size()) + " colors 4\n";
    res += " " + toString(data->texs->size()) + " texture coordinates\n";
    return res;
}

VRGeoData::PrimItr::PrimItr(VRGeoData* d, Primitive* p) { data = d; itr = p; }

int VRGeoData::primN(int type) {
    if (type == GL_POINTS) return 1;
    if (type == GL_LINES) return 2;
    if (type == GL_TRIANGLES) return 3;
    if (type == GL_QUADS) return 4;
    cout << "VRGeoData::primN WARNING: unknown GL type " << type << endl;
    return 0;
}

void VRGeoData::setIndices(Primitive& p) {
    vector<int> inds;

    int t = data->types->getValue(p.tID);
    int l = data->lengths->getValue(p.tID);
    int Np = primN(t);
    for (int j=0; j<Np; j++) {
        int k = p.lID + j;
        int i = data->indices->getValue(k);
        inds.push_back(i);
    }
    p.lID += Np;
    if (p.lID >= l) {
        p.tID++;
        p.lID = 0;
    }

    p.type = t;
    p.indices = inds;
}

VRGeoData::Primitive* VRGeoData::next() {
    if (!valid()) return 0;
    setIndices(current);
    //cout << "NEXT " << current.tID << " " << data->types->size() << " " << current.lID << " " << data->lengths->getValue(current.tID) << endl;
    if (current.tID >= data->types->size()) return 0;
    return &current;
}

VRGeoData::PrimItr VRGeoData::begin() {
    if (!valid()) return end();
    current.tID = 0;
    current.lID = 0;
    setIndices(current);
    return PrimItr( this, &current );
}

VRGeoData::PrimItr VRGeoData::begin() const { return begin(); }
VRGeoData::PrimItr VRGeoData::cbegin() const { return begin(); }
VRGeoData::PrimItr VRGeoData::end() { return PrimItr(this, 0); }
VRGeoData::PrimItr VRGeoData::end() const { return pend; }
VRGeoData::PrimItr VRGeoData::cend() const { return pend; }


