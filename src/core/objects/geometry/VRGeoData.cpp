#include "VRGeoData.h"
#include "VRGeometry.h"
#include "OSGGeometry.h"
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
    GeoVec2fPropertyRecPtr texs2;

    int lastPrim = -1;

    int getColorChannels(GeoVectorProperty* v) {
        if (v == 0) return 0;

        static map<int, int> channels;
        if (channels.size() == 0) {
            channels[ GeoVec2fProperty::create()->getType().getId() ] = 2;
            channels[ GeoVec3fProperty::create()->getType().getId() ] = 3;
            channels[ GeoVec4fProperty::create()->getType().getId() ] = 4;
            channels[ GeoColor3fProperty::create()->getType().getId() ] = 3;
            channels[ GeoColor4fProperty::create()->getType().getId() ] = 4;
            channels[ GeoVec2dProperty::create()->getType().getId() ] = 2;
            channels[ GeoVec3dProperty::create()->getType().getId() ] = 3;
            channels[ GeoVec4dProperty::create()->getType().getId() ] = 4;
        }

        int type = v->getType().getId();
        if (channels.count(type)) return channels[type];
        cout << "getColorChannels WARNING: unknown type ID " << type << endl;
        return 0;
    }
};

VRGeoData::VRGeoData() : pend(this, 0) { data = shared_ptr<Data>(new Data()); reset(); }

VRGeoData::VRGeoData(VRGeometryPtr geo) : pend(this, 0) {
    data = shared_ptr<Data>(new Data());
    if (!geo) { reset(); return; }

    this->geo = geo;
    data->types = (GeoUInt8Property*)geo->getMesh()->geo->getTypes();
    data->lengths = (GeoUInt32Property*)geo->getMesh()->geo->getLengths();
    data->indices = (GeoUInt32Property*)geo->getMesh()->geo->getIndices();
    data->pos = (GeoPnt3fProperty*)geo->getMesh()->geo->getPositions();
    data->norms = (GeoVec3fProperty*)geo->getMesh()->geo->getNormals();
    data->texs = (GeoVec2fProperty*)geo->getMesh()->geo->getTexCoords();
    data->texs2 = (GeoVec2fProperty*)geo->getMesh()->geo->getTexCoords1();
    auto cols = geo->getMesh()->geo->getColors();
    int Nc = data->getColorChannels(cols);
    if (Nc == 3) data->cols3 = (GeoVec3fProperty*)cols;
    if (Nc == 4) data->cols4 = (GeoVec4fProperty*)cols;
    if (!data->types) data->types = GeoUInt8Property::create();
    if (!data->lengths) data->lengths = GeoUInt32Property::create();
    if (!data->indices) data->indices = GeoUInt32Property::create();
    if (!data->pos) data->pos = GeoPnt3fProperty::create();
    if (!data->norms) data->norms = GeoVec3fProperty::create();
    if (!data->cols3) data->cols3 = GeoVec3fProperty::create();
    if (!data->cols4) data->cols4 = GeoVec4fProperty::create();
    if (!data->texs) data->texs = GeoVec2fProperty::create();
    if (!data->texs2) data->texs2 = GeoVec2fProperty::create();

    auto normsIdx = geo->getMesh()->geo->getIndex(Geometry::NormalsIndex);
    auto posIdx = geo->getMesh()->geo->getIndex(Geometry::PositionsIndex);
    if (normsIdx != posIdx) { // TODO: fix normals
        //map<int, int> mapping;
        cout << "VRGeoData Warning: normals and positions dont share indices!\n";
    }
}

VRGeoDataPtr VRGeoData::create() { return VRGeoDataPtr( new VRGeoData() ); }

void VRGeoData::reset() {
    if (data->types) data->types->clear();
    else data->types = GeoUInt8Property::create();
    if (data->lengths) data->lengths->clear();
    else data->lengths = GeoUInt32Property::create();
    if (data->indices) data->indices->clear();
    else data->indices = GeoUInt32Property::create();
    if (data->pos) data->pos->clear();
    else data->pos = GeoPnt3fProperty::create();
    if (data->norms) data->norms->clear();
    else data->norms = GeoVec3fProperty::create();
    if (data->cols3) data->cols3->clear();
    else data->cols3 = GeoVec3fProperty::create();
    if (data->cols4) data->cols4->clear();
    else data->cols4 = GeoVec4fProperty::create();
    if (data->texs) data->texs->clear();
    else data->texs = GeoVec2fProperty::create();
    if (data->texs2) data->texs2->clear();
    else data->texs2 = GeoVec2fProperty::create();
    data->lastPrim = -1;
}

bool VRGeoData::valid() const {
    if (!data->types->size()) { cout << "VRGeoData invalid: no types!\n"; return false; }
    if (!data->lengths->size()) { cout << "VRGeoData invalid: no lengths!\n"; return false; }
    if (!data->pos->size()) { cout << "VRGeoData invalid: no pos!\n"; return false; }
    return true;
}

int VRGeoData::size() const { return data->pos->size(); }

Pnt3f VRGeoData::getPosition(int i) { return int(data->pos->size()) > i ? data->pos->getValue(i) : Pnt3f(); }
Vec3f VRGeoData::getNormal(int i) { return int(data->norms->size()) > i ? data->norms->getValue(i) : Vec3f(); }
Vec4f VRGeoData::getColor(int i) {
    if (int(data->cols4->size()) > i) return data->cols4->getValue(i);
    if (int(data->cols3->size()) > i) {
        Vec3f c = data->cols3->getValue(i);
        return Vec4f( c[0], c[1], c[2], 1.0 );
    }
    return Vec4f();
}

int VRGeoData::pushVert(Pnt3f p) { data->pos->addValue(p); return data->pos->size()-1; }
int VRGeoData::pushVert(Pnt3f p, Vec3f n) { data->norms->addValue(n); return pushVert(p); }
int VRGeoData::pushVert(Pnt3f p, Vec3f n, Vec3f c) { data->cols3->addValue(c); return pushVert(p,n); }
int VRGeoData::pushVert(Pnt3f p, Vec3f n, Vec4f c) { data->cols4->addValue(c); return pushVert(p,n); }
int VRGeoData::pushVert(Pnt3f p, Vec3f n, Vec2f t) { data->texs->addValue(t); return pushVert(p,n); }
int VRGeoData::pushVert(Pnt3f p, Vec3f n, Vec2f t, Vec2f t2) { data->texs2->addValue(t2); return pushVert(p,n,t); }
int VRGeoData::pushVert(Pnt3f p, Vec3f n, Vec3f c, Vec2f t) { data->texs->addValue(t); return pushVert(p,n,c); }
int VRGeoData::pushVert(Pnt3f p, Vec3f n, Vec4f c, Vec2f t) { data->texs->addValue(t); return pushVert(p,n,c); }
int VRGeoData::pushVert(Pnt3f p, Vec3f n, Vec3f c, Vec2f t, Vec2f t2) { data->texs2->addValue(t2); return pushVert(p,n,c,t); }
int VRGeoData::pushVert(Pnt3f p, Vec3f n, Vec4f c, Vec2f t, Vec2f t2) { data->texs2->addValue(t2); return pushVert(p,n,c,t); }

int VRGeoData::pushColor(Vec3f c) { data->cols3->addValue(c); return data->cols3->size()-1; }
int VRGeoData::pushColor(Vec4f c) { data->cols4->addValue(c); return data->cols4->size()-1; }

bool VRGeoData::setVert(int i, Pnt3f p) { if (size() > i) data->pos->setValue(p,i); else return 0; return 1; }
bool VRGeoData::setVert(int i, Pnt3f p, Vec3f n) { if (size() > i) data->norms->setValue(n,i); else return 0; return setVert(i,p); }
bool VRGeoData::setVert(int i, Pnt3f p, Vec3f n, Vec3f c) { if (size() > i) data->cols3->setValue(c,i); else return 0; return setVert(i,p,n); }
bool VRGeoData::setVert(int i, Pnt3f p, Vec3f n, Vec4f c) { if (size() > i) data->cols4->setValue(c,i); else return 0; return setVert(i,p,n); }
bool VRGeoData::setVert(int i, Pnt3f p, Vec3f n, Vec2f t) { if (size() > i) data->texs->setValue(t,i); else return 0; return setVert(i,p,n); }
bool VRGeoData::setVert(int i, Pnt3f p, Vec3f n, Vec2f t, Vec2f t2) { if (size() > i) data->texs2->setValue(t2,i); else return 0; return setVert(i,p,n,t); }
bool VRGeoData::setVert(int i, Pnt3f p, Vec3f n, Vec3f c, Vec2f t) { if (size() > i) data->texs->setValue(t,i); else return 0; return setVert(i,p,n,c); }
bool VRGeoData::setVert(int i, Pnt3f p, Vec3f n, Vec4f c, Vec2f t) { if (size() > i) data->texs->setValue(t,i); else return 0; return setVert(i,p,n,c); }
bool VRGeoData::setVert(int i, Pnt3f p, Vec3f n, Vec3f c, Vec2f t, Vec2f t2) { if (size() > i) data->texs2->setValue(t2,i); else return 0; return setVert(i,p,n,c,t); }
bool VRGeoData::setVert(int i, Pnt3f p, Vec3f n, Vec4f c, Vec2f t, Vec2f t2) { if (size() > i) data->texs2->setValue(t2,i); else return 0; return setVert(i,p,n,c,t); }

void VRGeoData::pushQuad(Vec3f p, Vec3f n, Vec3f u, Vec2f s, bool addInds) {
    Vec3f x = n.cross(u); x.normalize();
    pushVert(p - x*s[0]*0.5 - u*s[1]*0.5, n, Vec2f(0,0));
    pushVert(p + x*s[0]*0.5 - u*s[1]*0.5, n, Vec2f(1,0));
    pushVert(p + x*s[0]*0.5 + u*s[1]*0.5, n, Vec2f(1,1));
    pushVert(p - x*s[0]*0.5 + u*s[1]*0.5, n, Vec2f(0,1));
    if (addInds) pushQuad();
}

int VRGeoData::pushVert(const VRGeoData& other, int i) {
    auto od = other.data;
    if (int(od->pos->size()) <= i) { cout << "VRGeoData::pushVert ERROR: invalid index " << i << endl; return 0; }
    Pnt3f p = od->pos->getValue(i);
    bool doNorms = (od->norms && int(od->norms->size()) > i);
    bool doCol3 = (od->cols3 && int(od->cols3->size()) > i);
    bool doCol4 = (od->cols4 && int(od->cols4->size()) > i);
    bool doTex = (od->texs && int(od->texs->size()) > i);
    bool doTex2 = (od->texs2 && int(od->texs2->size()) > i);
    if (doNorms) {
        Vec3f n = od->norms->getValue(i);
        if (doTex2 && doCol3) return pushVert(p, n, od->cols3->getValue(i), od->texs->getValue(i), od->texs2->getValue(i));
        if (doTex2 && doCol4) return pushVert(p, n, od->cols4->getValue(i), od->texs->getValue(i), od->texs2->getValue(i));
        if (doTex && doCol3) return pushVert(p, n, od->cols3->getValue(i), od->texs->getValue(i));
        if (doTex && doCol4) return pushVert(p, n, od->cols4->getValue(i), od->texs->getValue(i));
        if (doTex2) return pushVert(p, n, od->texs->getValue(i), od->texs2->getValue(i));
        if (doTex)  return pushVert(p, n, od->texs->getValue(i));
        if (doCol3) return pushVert(p, n, od->cols3->getValue(i));
        if (doCol4) return pushVert(p, n, od->cols4->getValue(i));
        return pushVert(p, n);
    }
    return pushVert(p);
}

int VRGeoData::pushVert(const VRGeoData& other, int i, Matrix m) {
    auto od = other.data;
    if (int(od->pos->size()) <= i) { cout << "VRGeoData::pushVert ERROR: invalid index " << i << endl; return 0; }
    Pnt3f p = od->pos->getValue(i);
    m.mult(p,p);
    bool doNorms = (od->norms && int(od->norms->size()) > i);
    bool doCol3 = (od->cols3 && int(od->cols3->size()) > i);
    bool doCol4 = (od->cols4 && int(od->cols4->size()) > i);
    bool doTex = (od->texs && int(od->texs->size()) > i);
    bool doTex2 = (od->texs2 && int(od->texs2->size()) > i);
    if (doNorms) {
        Vec3f n = od->norms->getValue(i);
        m.mult(n,n);
        if (doTex2 && doCol3) return pushVert(p, n, od->cols3->getValue(i), od->texs->getValue(i), od->texs2->getValue(i));
        if (doTex2 && doCol4) return pushVert(p, n, od->cols4->getValue(i), od->texs->getValue(i), od->texs2->getValue(i));
        if (doTex && doCol3) return pushVert(p, n, od->cols3->getValue(i), od->texs->getValue(i));
        if (doTex && doCol4) return pushVert(p, n, od->cols4->getValue(i), od->texs->getValue(i));
        if (doTex2) return pushVert(p, n, od->texs->getValue(i), od->texs2->getValue(i));
        if (doTex)  return pushVert(p, n, od->texs->getValue(i));
        if (doCol3) return pushVert(p, n, od->cols3->getValue(i));
        if (doCol4) return pushVert(p, n, od->cols4->getValue(i));
        return pushVert(p, n);
    }
    return pushVert(p);
}

bool VRGeoData::isStripOrFan(int t) {
    if (t == GL_LINE_LOOP || t == GL_LINE_STRIP) return true;
    if (t == GL_TRIANGLE_FAN || t == GL_TRIANGLE_STRIP) return true;
    if (t == GL_QUAD_STRIP) return true;
    return false;
}

void VRGeoData::extentType(int N) {
    int i = data->lengths->size()-1;
    int l = data->lengths->getValue(i);
    data->lengths->setValue(l+N, i);
}

void VRGeoData::updateType(int t, int N) {
    if (data->lastPrim == t) { extentType(N); return; }
    data->types->addValue(t);
    data->lastPrim = t;
    data->lengths->addValue(N);
}

void VRGeoData::pushQuad(int i, int j, int k, int l) {
    int N = size();
    if (i < 0) i += N;
    if (j < 0) j += N;
    if (k < 0) k += N;
    if (l < 0) l += N;
    data->indices->addValue(i);
    data->indices->addValue(j);
    data->indices->addValue(k);
    data->indices->addValue(l);
    updateType(GL_QUADS, 4);
}

void VRGeoData::pushPatch(int N) {
    int vN = size();
    for (int i=0; i<N; i++) data->indices->addValue(vN-N+i);
    updateType(GL_PATCHES, N);
}

void VRGeoData::pushTri(int i, int j, int k) {
    int N = size();
    if (i < 0) i += N;
    if (j < 0) j += N;
    if (k < 0) k += N;
    data->indices->addValue(i);
    data->indices->addValue(j);
    data->indices->addValue(k);
    updateType(GL_TRIANGLES, 3);
}

void VRGeoData::pushLine(int i, int j) {
    int N = size();
    if (i < 0) i += N;
    if (j < 0) j += N;
    data->indices->addValue(i);
    data->indices->addValue(j);
    updateType(GL_LINES, 2);
}

void VRGeoData::pushPoint(int i) {
    int N = size();
    if (i < 0) i += N;
    data->indices->addValue(i);
    updateType(GL_POINTS, 1);
}

void VRGeoData::pushLine() { int N = size(); if (N > 1) pushLine(N-2, N-1); }
void VRGeoData::pushTri() { int N = size(); if (N > 2) pushTri(N-3, N-2, N-1); }
void VRGeoData::pushQuad() { int N = size(); if (N > 3) pushQuad(N-4, N-3, N-2, N-1); }

void VRGeoData::pushPrim(Primitive p) {
    int No = primNOffset(p.lid, p.type);
    int N = p.indices.size();
    //int iN0 = size();
    //cout << "pushPrim: " << p.asString() << endl;
    //cout << "pushPrim: " << No << " " << p.lid << " " << iN0 << endl;
    for (int i = No; i<N; i++) {
        data->indices->addValue( p.indices[i] );
        //cout << "add index: " << p.indices[i] << endl;
    }
    if (p.lid == 0 && isStripOrFan(p.type)) data->lastPrim = -1;
    updateType(p.type, N-No);
}

void VRGeoData::apply(VRGeometryPtr geo, bool check) const {
    if (!geo) { cout << "VRGeoData::apply to geometry " << geo->getName() << " failed: geometry invalid!" << endl; return; }
    if (!valid() && check) { cout << "VRGeoData::apply to geometry " << geo->getName() << " failed: data invalid!" << endl; return; }
    geo->setPositions(data->pos);
    if (data->lengths->size() > 0) geo->setLengths(data->lengths);
    if (data->types->size() > 0) geo->setTypes(data->types);
    if (data->norms->size() > 0) geo->setNormals(data->norms);
    if (data->indices->size() > 0) geo->setIndices(data->indices);
    if (data->cols3->size() > 0) geo->setColors(data->cols3);
    if (data->cols4->size() > 0) geo->setColors(data->cols4);
    if (data->texs->size() > 0) geo->setTexCoords(data->texs, 0);
    if (data->texs2->size() > 0) geo->setTexCoords(data->texs2, 1);
}

void VRGeoData::append(const VRGeoData& geo, const Matrix& m) {
    map<int, int> mapping;
    for (auto p : geo) {
        vector<int> ninds;
        for (auto i : p.indices) {
            if (!mapping.count(i)) mapping[i] = pushVert(geo, i, m);
            ninds.push_back(mapping[i]);
        }
        p.indices = ninds;
        pushPrim(p);
    }
}

void VRGeoData::append(VRGeometryPtr geo, const Matrix& m) { append( VRGeoData(geo), m ); }

VRGeometryPtr VRGeoData::asGeometry(string name) const {
    auto geo = VRGeometry::create(name);
    apply(geo);
    return geo;
}

string VRGeoData::status() {
    string res;
    res += "VRGeoData stats:\n";
    res += " " + toString(data->types->size()) + " types: ";
    for (uint i=0; i<data->types->size(); i++) res += " " + toString(data->types->getValue(i));
    res += "\n";
    res += " " + toString(data->lengths->size()) + " lengths: ";
    for (uint i=0; i<data->lengths->size(); i++) res += " " + toString(data->lengths->getValue(i));
    res += "\n";
    res += " " + toString(data->pos->size()) + " positions\n";
    for (uint i=0; i<data->pos->size(); i++) res += " " + toString(data->pos->getValue(i));
    res += "\n";
    res += " " + toString(data->norms->size()) + " normals\n";
    res += " " + toString(data->cols3->size()) + " colors 3\n";
    res += " " + toString(data->cols4->size()) + " colors 4\n";
    res += " " + toString(data->texs->size()) + " texture coordinates\n";
    res += " " + toString(data->texs2->size()) + " texture coordinates 2\n";
    res += " " + toString(data->indices->size()) + " indices: ";
    for (uint i=0; i<data->indices->size(); i++) res += " " + toString(data->indices->getValue(i));
    res += "\n";
    return res;
}

VRGeoData::PrimItr::PrimItr(const VRGeoData* d, Primitive* p) { data = d; itr = p; }

int VRGeoData::primN(int type) const {
    if (type == GL_POINTS) return 1;
    if (type == GL_LINES) return 2;
    if (type == GL_TRIANGLES) return 3;
    if (type == GL_QUADS) return 4;

    if (type == GL_LINE_LOOP) return 2;
    if (type == GL_LINE_STRIP) return 2;
    if (type == GL_TRIANGLE_FAN) return 3;
    if (type == GL_TRIANGLE_STRIP) return 3;
    if (type == GL_QUAD_STRIP) return 4;
    //if (type == GL_POLYGON) return 0; // TODO

    cout << "VRGeoData::primN WARNING: unknown GL type " << type << endl;
    return 0;
}

int VRGeoData::primNOffset(int lID, int type) const {
    if (lID == 0) return 0;

    if (type == GL_POINTS) return 0;
    if (type == GL_LINES) return 0;
    if (type == GL_TRIANGLES) return 0;
    if (type == GL_QUADS) return 0;

    if (type == GL_LINE_LOOP) return 1;
    if (type == GL_LINE_STRIP) return 1;
    if (type == GL_TRIANGLE_FAN) return 2;
    if (type == GL_TRIANGLE_STRIP) return 2;
    if (type == GL_QUAD_STRIP) return 2;
    //if (type == GL_POLYGON) return 0; // TODO

    cout << "VRGeoData::primN WARNING: unknown GL type " << type << endl;
    return 0;
}

bool VRGeoData::setIndices(Primitive& p) const {
    vector<int> inds;

    if (p.tID >= int(data->types->size())) return false;

    int t = data->types->getValue(p.tID);
    int l = data->lengths->getValue(p.tID);
    int Np = primN(t);
    int Npo = primNOffset(p.lID, t);
    for (int j=0; j<Np; j++) {
        int k = p.pID + j - Npo;
        int i = data->indices->getValue(k);
        inds.push_back(i);
    }
    p.type = t;
    p.indices = inds;
    p.lid = p.lID;
    p.tid = p.tID;
    p.lID += Np - Npo;
    p.pID += Np - Npo;
    if (p.lID >= l) {
        p.tID++;
        p.lID = 0;
    }
    return true;
}

int VRGeoData::getNIndices() { return data->indices->size(); }

VRGeoData::Primitive* VRGeoData::next() const {
    if (!valid()) return 0;
    if (!setIndices(current)) return 0;
    return &current;
}

VRGeoData::PrimItr VRGeoData::begin() {
    if (!valid()) return end();
    current.tID = 0;
    current.lID = 0;
    if (!setIndices(current)) return end();
    return PrimItr( this, &current );
}

VRGeoData::PrimItr VRGeoData::begin() const {
    if (!valid()) return end();
    current.tID = 0;
    current.lID = 0;
    if (!setIndices(current)) return end();
    return PrimItr( this, &current );
}

VRGeoData::PrimItr VRGeoData::cbegin() const { return begin(); }
VRGeoData::PrimItr VRGeoData::end() { return PrimItr(this, 0); }
VRGeoData::PrimItr VRGeoData::end() const { return pend; }
VRGeoData::PrimItr VRGeoData::cend() const { return pend; }

string VRGeoData::Primitive::asString() {
    string res;
    int N = indices.size();
    res = "prim " + toString(type) + " " + toString(tid) + " " + toString(lid) + " " + toString(N) + ": ";
    for (auto i : indices) res += " " + toString(i);
    return res;
}

void VRGeoData::test_copy(VRGeoData& g) {
    return;
    //data->types = g.data->types;
    //data->lengths = g.data->lengths;

    data->types->clear();
    data->lengths->clear();

    // GL_TRIANGLE_STRIP 5
    // GL_TRIANGLE_FAN 6

    for (uint i=0; i<g.data->types->size(); i++) {
        int t = g.data->types->getValue(i);
        int l = g.data->lengths->getValue(i);
        data->types->addValue(t);
        data->lengths->addValue(l);
        cout << " tl " << t << " " << l << endl;
    }

    /*data->types->clear();
    data->types->addValue(0x0005);
    data->types->addValue(0);
    data->types->addValue(0);
    data->types->addValue(0);
    data->types->addValue(0x0005);
    data->types->addValue(0);
    data->types->addValue(0);
    data->types->addValue(0);
    data->types->addValue(0x0006);
    data->types->addValue(0);
    data->types->addValue(0);*/


    //data->indices = g.data->indices;
    /*data->pos = g.data->pos;
    data->norms = g.data->norms;
    data->cols3 = g.data->cols3;
    data->cols4 = g.data->cols4;
    data->texs = g.data->texs;*/
}

void VRGeoData::addVertexColors(Vec3f c) {
    int N = size();
    auto& cols = data->cols3;
    if (cols->size() == 0) {
        for (int i=0; i<N; i++) cols->addValue(c);
    }
    if (geo) {
        geo->fixColorMapping();
        geo->getMesh()->geo->setColors(cols);
    }
}



