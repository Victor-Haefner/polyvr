#include "VRGeoData.h"
#include "VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "OSGGeometry.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGTriangleIterator.h>
#include <OpenSG/OSGNameAttachment.h>

using namespace OSG;

struct VRGeoData::Data {
    GeoUInt8PropertyMTRecPtr types;
    GeoUInt32PropertyMTRecPtr lengths;
    GeoUInt32PropertyMTRecPtr indices;
    GeoPnt3fPropertyMTRecPtr pos;
    GeoVec3fPropertyMTRecPtr norms;
    GeoVec3fPropertyMTRecPtr cols3;
    GeoVec4fPropertyMTRecPtr cols4;
    GeoVec2fPropertyMTRecPtr texs;
    GeoVec2fPropertyMTRecPtr texs2;
    GeoUInt32PropertyMTRecPtr indicesNormals;
    GeoUInt32PropertyMTRecPtr indicesColors;
    GeoUInt32PropertyMTRecPtr indicesTexCoords;

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
#ifndef WASM
            channels[ GeoVec2dProperty::create()->getType().getId() ] = 2;
            channels[ GeoVec3dProperty::create()->getType().getId() ] = 3;
            channels[ GeoVec4dProperty::create()->getType().getId() ] = 4;
#endif
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
    if (geo->getMesh() && geo->getMesh()->geo) {
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

        data->indicesNormals = (GeoUInt32Property*)geo->getMesh()->geo->getIndex(Geometry::NormalsIndex);
        data->indicesColors = (GeoUInt32Property*)geo->getMesh()->geo->getIndex(Geometry::ColorsIndex);
        data->indicesTexCoords = (GeoUInt32Property*)geo->getMesh()->geo->getIndex(Geometry::TexCoordsIndex);
    }

    if (data->types && data->types->size() > 0) data->lastPrim = data->types->getValue( data->types->size()-1 );
    if (!data->types) { data->types = GeoUInt8Property::create(); OSG::setName( data->types, "GeoData_types"); }
    if (!data->lengths) data->lengths = GeoUInt32Property::create();
    if (!data->indices) data->indices = GeoUInt32Property::create();
    if (!data->pos) data->pos = GeoPnt3fProperty::create();
    if (!data->norms) data->norms = GeoVec3fProperty::create();
    if (!data->cols3) data->cols3 = GeoVec3fProperty::create();
    if (!data->cols4) data->cols4 = GeoVec4fProperty::create();
    if (!data->texs) data->texs = GeoVec2fProperty::create();
    if (!data->texs2) data->texs2 = GeoVec2fProperty::create();
    if (!data->indicesNormals) data->indicesNormals = GeoUInt32Property::create();
    if (!data->indicesColors) data->indicesColors = GeoUInt32Property::create();
    if (!data->indicesTexCoords) data->indicesTexCoords = GeoUInt32Property::create();

    // TODO: might not be really possible.. ..maybe live with it and check that all algorithms take it into account!
    /*auto nIdx = geo->getMesh()->geo->getIndex(Geometry::NormalsIndex);
    auto pIdx = geo->getMesh()->geo->getIndex(Geometry::PositionsIndex);
    if (nIdx != pIdx) {
        GeoVec3fPropertyMTRecPtr norms  = (GeoVec3fProperty*)geo->getMesh()->geo->getNormals();
        GeoVec3fPropertyMTRecPtr norms2 = GeoVec3fProperty::create();

        for (unsigned int i=0; i<pIdx->size(); i++) {
            int pID = pIdx->getValue(i);
            int nID = nIdx->getValue(i);
            if (norms2->size() <= pID) norms2->resize(pID+1);
            norms2->setValue( norms->getValue(nID), pID );
        }

        geo->setNormals(norms2);
        geo->getMesh()->geo->setIndex(pIdx, Geometry::NormalsIndex);
    }*/
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
    if (data->indicesNormals) data->indicesNormals->clear();
    else data->indicesNormals = GeoUInt32Property::create();
    if (data->indicesColors) data->indicesColors->clear();
    else data->indicesColors = GeoUInt32Property::create();
    if (data->indicesTexCoords) data->indicesTexCoords->clear();
    else data->indicesTexCoords = GeoUInt32Property::create();
    data->lastPrim = -1;
}

bool VRGeoData::valid() const {
    if (!data->types->size()) { cout << "VRGeoData invalid: no types!\n"; return false; }
    if (!data->lengths->size()) { cout << "VRGeoData invalid: no lengths!\n"; return false; }
    if (!data->pos->size()) { cout << "VRGeoData invalid: no pos!\n"; return false; }

    int Ni = data->indices->size();
    int Nni = data->indicesNormals->size();
    int Nci = data->indicesColors->size();
    int Nti = data->indicesTexCoords->size();
    if (Ni > 0) {
        if (Nni > 0 && Nni != Ni) { cout << "VRGeoData invalid: coord and normal indices lengths mismatch!\n"; return false; }
        if (Nci > 0 && Nci != Ni) { cout << "VRGeoData invalid: coord and color indices lengths mismatch!\n"; return false; }
        if (Nti > 0 && Nti != Ni) { cout << "VRGeoData invalid: coord and texcoords indices lengths mismatch!\n"; return false; }
    } else {
        if (Nni > 0) { cout << "VRGeoData invalid: normal indices defined but no coord indices!\n"; return false; }
        if (Nci > 0) { cout << "VRGeoData invalid: color indices defined but no coord indices!\n"; return false; }
        if (Nti > 0) { cout << "VRGeoData invalid: texcoords indices defined but no coord indices!\n"; return false; }
    }
    return true;
}

bool VRGeoData::validIndices() const {
    auto checkMaxIndex = [](GeoUInt32PropertyMTRecPtr indices, unsigned int VecN) {
        if (VecN == 0) return true;
        unsigned int Imax = 0;
        for (unsigned int i = 0; i < indices->size(); i++) {
            unsigned int idx = indices->getValue(i);
            if (idx > Imax) Imax = idx;
        }
        if (Imax >= VecN) return false;
        return true;
    };

    if (!checkMaxIndex(data->indices, data->pos->size())) { cout << "VRGeoData invalid: coord indices have too big values!\n"; return false; }
    if (!checkMaxIndex(data->indicesNormals, data->norms->size())) { cout << "VRGeoData invalid: normal indices have too big values!\n"; return false; }
    if (!checkMaxIndex(data->indicesTexCoords, data->texs->size())) { cout << "VRGeoData invalid: tex coords indices have too big values!\n"; return false; }
    if (data->cols3->size())
        if (!checkMaxIndex(data->indicesColors, data->cols3->size())) { cout << "VRGeoData invalid: color3 indices have too big values!\n"; return false; }
    if (data->cols4->size())
        if (!checkMaxIndex(data->indicesColors, data->cols4->size())) { cout << "VRGeoData invalid: color4 indices have too big values!\n"; return false; }
    return true;
}

void VRGeoData::apply(VRGeometryPtr geo, bool check, bool checkIndices) const {
    if (!geo) { cout << "VRGeoData::apply to geometry " << geo->getName() << " failed: geometry invalid!" << endl; return; }
    if (check && !valid()) { cout << "VRGeoData::apply to geometry " << geo->getName() << " failed: data invalid!" << endl; return; }
    if (checkIndices && !validIndices()) { cout << "VRGeoData::apply to geometry " << geo->getName() << " failed: indices invalid!" << endl; return; }

    geo->setPositions( data->pos );
    geo->setLengths( data->lengths->size() > 0 ? data->lengths : 0 );
    geo->setTypes( data->types->size() > 0 ? data->types : 0 );
    geo->setNormals( data->norms->size() > 0 ? data->norms : 0 );
    geo->setTexCoords( data->texs->size() > 0 ? data->texs : 0, 0 );
    geo->setTexCoords( data->texs2->size() > 0 ? data->texs2 : 0, 1 );
    if (data->indices->size() > 0) geo->setIndices( data->indices );

    if (data->indicesNormals->size() > 0) {
        if (data->indicesNormals->size() == data->indices->size()) {
            geo->getMesh()->geo->setIndex( data->indicesNormals, Geometry::NormalsIndex );
            geo->getMesh()->geo->addAttachment(data->indicesNormals, 61);
        }
    }

    if (data->indicesColors->size() > 0) {
        if (data->indicesColors->size() == data->indices->size()) {
            geo->getMesh()->geo->setIndex( data->indicesColors, Geometry::ColorsIndex );
            geo->getMesh()->geo->addAttachment(data->indicesColors, 62);
        }
    }

    if (data->indicesTexCoords->size() > 0) {
        if (data->indicesTexCoords->size() == data->indices->size()) {
            geo->getMesh()->geo->setIndex( data->indicesTexCoords, Geometry::TexCoordsIndex );
            geo->getMesh()->geo->addAttachment(data->indicesTexCoords, 63);
        }
    }

    GeoVectorProperty* c3 = data->cols3->size() > 0 ? data->cols3 : 0;
    GeoVectorProperty* c4 = data->cols4->size() > 0 ? data->cols4 : 0;
    geo->setColors( c3 ? c3 : c4 );
}

int VRGeoData::size() const { return data->pos->size(); }
int VRGeoData::sizeNormals() const { return data->norms->size(); }
int VRGeoData::getNTypes() const { return data->types->size(); }

int VRGeoData::getNFaces() const {
    int N = 0;
    for (unsigned int i=0; i<data->types->size(); i++) {
        int t = data->types->getValue(i);
        int l = data->lengths->getValue(i);
        if (t == GL_TRIANGLES) N += l/3;
        if (t == GL_QUADS) N += l/4;
    }
    return N;
}

int VRGeoData::getFaceSize(int fID) const {
    int n = 0;
    int N = 0;
    for (unsigned int i=0; i<data->types->size(); i++) {
        int t = data->types->getValue(i);
        int l = data->lengths->getValue(i);
        if (t == GL_TRIANGLES) { N += l/3; n = 3; }
        if (t == GL_QUADS) { N += l/4; n = 4; }
        if (fID < N) return n;
    }
    return 0;
}

int VRGeoData::getType(int i) { return int(data->types->size()) > i ? data->types->getValue(i) : 0; }
int VRGeoData::getLength(int i) { return int(data->lengths->size()) > i ? data->lengths->getValue(i) : 0; }

int VRGeoData::getIndex(int i, int v) {
    if (v == NormalsIndex && data->indicesNormals) return int(data->indicesNormals->size()) > i ? data->indices->getValue(i) : 0;
    if (v == ColorsIndex && data->indicesColors) return int(data->indicesColors->size()) > i ? data->indices->getValue(i) : 0;
    if (v == TexCoordsIndex && data->indicesTexCoords) return int(data->indicesTexCoords->size()) > i ? data->indices->getValue(i) : 0;
    return int(data->indices->size()) > i ? data->indices->getValue(i) : 0;
}

Pnt3d VRGeoData::getPosition(int i) { return int(data->pos->size()) > i ? Pnt3d(data->pos->getValue(i)) : Pnt3d(); }
Vec3d VRGeoData::getNormal(int i) { return int(data->norms->size()) > i ? Vec3d(data->norms->getValue(i)) : Vec3d(); }
Color4f VRGeoData::getColor(int i) {
    if (int(data->cols4->size()) > i) return data->cols4->getValue(i);
    if (int(data->cols3->size()) > i) {
        auto c = data->cols3->getValue(i);
        return Color4f( c[0], c[1], c[2], 1.0 );
    }
    return Color4f();
}

Color3f VRGeoData::getColor3(int i) {
    if (int(data->cols3->size()) > i) return data->cols3->getValue(i);
    if (int(data->cols4->size()) > i) {
        auto c = data->cols4->getValue(i);
        return Color3f( c[0], c[1], c[2]);
    }
    return Color3f();
}

string VRGeoData::getDataName(int type) {
    if (type == 0) return "types";
    if (type == 1) return "lengths";
    if (type == 2) return "indices";
    if (type == 3) return "positions";
    if (type == 4) return "normals";
    if (type == 5) return "RGB colors";
    if (type == 6) return "RGBA colors";
    if (type == 7) return "texture coords";
    if (type == 8) return "texture coords 2";
    if (type == 9) return "normals indices";
    if (type == 10) return "colors indices";
    return "";
}

int VRGeoData::getDataSize(int type) {
    if (type == 0) return data->types->size();
    if (type == 1) return data->lengths->size();
    if (type == 2) return data->indices->size();
    if (type == 3) return data->pos->size();
    if (type == 4) return data->norms->size();
    if (type == 5) return data->cols3->size();
    if (type == 6) return data->cols4->size();
    if (type == 7) return data->texs->size();
    if (type == 8) return data->texs2->size();
    if (type == 9) return data->indicesNormals->size();
    if (type == 10) return data->indicesColors->size();
    if (type == 11) return data->indicesTexCoords->size();
    return 0;
}

template<class T, typename P> string propToString(P p) {
    if (!p) return "";
    stringstream ss;
    ss << "[";
    for (unsigned int i=0; i<p->size(); i++) {
        if (i > 0) ss << ", ";
        T t = T(p->getValue(i));
        ss << toString(t);
    }
    ss << "]";
    return ss.str();
}

template<> string toString(const GeoUInt8PropertyMTRecPtr& p) { return propToString<UInt8>(p); }
template<> string toString(const GeoUInt32PropertyMTRecPtr& p) { return propToString<UInt32>(p); }
template<> string toString(const GeoPnt3fPropertyMTRecPtr& p) { return propToString<Pnt3d>(p); }
template<> string toString(const GeoVec2fPropertyMTRecPtr& p) { return propToString<Vec2d>(p); }
template<> string toString(const GeoVec3fPropertyMTRecPtr& p) { return propToString<Vec3d>(p); }
template<> string toString(const GeoVec4fPropertyMTRecPtr& p) { return propToString<Vec4d>(p); }

string VRGeoData::getDataAsString(int type) {
    if (type == 0) return toString(data->types);
    if (type == 1) return toString(data->lengths);
    if (type == 2) return toString(data->indices);
    if (type == 3) return toString(data->pos);
    if (type == 4) return toString(data->norms);
    if (type == 5) return toString(data->cols3);
    if (type == 6) return toString(data->cols4);
    if (type == 7) return toString(data->texs);
    if (type == 8) return toString(data->texs2);
    if (type == 9) return toString(data->indicesNormals);
    if (type == 10) return toString(data->indicesColors);
    if (type == 11) return toString(data->indicesTexCoords);
    return "";
}

int VRGeoData::pushVert(Pnt3d p) { data->pos->addValue(p); return data->pos->size()-1; }
int VRGeoData::pushVert(Pnt3d p, Vec3d n) { data->norms->addValue(n); return pushVert(p); }
int VRGeoData::pushVert(Pnt3d p, Vec3d n, Color3f c) { data->cols3->addValue(c); return pushVert(p,n); }
int VRGeoData::pushVert(Pnt3d p, Vec3d n, Color4f c) { data->cols4->addValue(c); return pushVert(p,n); }
int VRGeoData::pushVert(Pnt3d p, Vec3d n, Vec2d t) { data->texs->addValue(t); return pushVert(p,n); }
int VRGeoData::pushVert(Pnt3d p, Vec3d n, Vec2d t, Vec2d t2) { data->texs2->addValue(t2); return pushVert(p,n,t); }
int VRGeoData::pushVert(Pnt3d p, Vec3d n, Color3f c, Vec2d t) { data->texs->addValue(t); return pushVert(p,n,c); }
int VRGeoData::pushVert(Pnt3d p, Vec3d n, Color4f c, Vec2d t) { data->texs->addValue(t); return pushVert(p,n,c); }
int VRGeoData::pushVert(Pnt3d p, Vec3d n, Color3f c, Vec2d t, Vec2d t2) { data->texs2->addValue(t2); return pushVert(p,n,c,t); }
int VRGeoData::pushVert(Pnt3d p, Vec3d n, Color4f c, Vec2d t, Vec2d t2) { data->texs2->addValue(t2); return pushVert(p,n,c,t); }

int VRGeoData::pushType(int t) { data->types->addValue(t); return data->types->size()-1; }
int VRGeoData::pushLength(int l) { data->lengths->addValue(l); return data->lengths->size()-1; }
int VRGeoData::pushIndex(int i) { data->indices->addValue(i); return data->indices->size()-1; }
int VRGeoData::pushPos(Pnt3d p) { data->pos->addValue(p); return data->pos->size()-1; }
int VRGeoData::pushNorm(Vec3d n) { data->norms->addValue(n); return data->norms->size()-1; }
int VRGeoData::pushTexCoord(Vec2d t) { data->texs->addValue(t); return data->texs->size()-1; }
int VRGeoData::pushTexCoord2(Vec2d t) { data->texs2->addValue(t); return data->texs2->size()-1; }
int VRGeoData::pushColor(Color3f c) { data->cols3->addValue(c); return data->cols3->size()-1; }
int VRGeoData::pushColor(Color4f c) { data->cols4->addValue(c); return data->cols4->size()-1; }

int VRGeoData::pushNormalIndex(int i) { data->indicesNormals->addValue(i); return data->indicesNormals->size()-1; }
int VRGeoData::pushColorIndex(int i) { data->indicesColors->addValue(i); return data->indicesColors->size()-1; }
int VRGeoData::pushTexCoordIndex(int i) { data->indicesTexCoords->addValue(i); return data->indicesTexCoords->size()-1; }

bool VRGeoData::setVert(int i, Pnt3d p) { if (size() > i) data->pos->setValue(p,i); else return 0; return 1; }
bool VRGeoData::setVert(int i, Pnt3d p, Vec3d n) { if (size() > i) data->norms->setValue(n,i); else return 0; return setVert(i,p); }
bool VRGeoData::setVert(int i, Pnt3d p, Vec3d n, Color3f c) { if (size() > i) data->cols3->setValue(c,i); else return 0; return setVert(i,p,n); }
bool VRGeoData::setVert(int i, Pnt3d p, Vec3d n, Color4f c) { if (size() > i) data->cols4->setValue(c,i); else return 0; return setVert(i,p,n); }
bool VRGeoData::setVert(int i, Pnt3d p, Vec3d n, Vec2d t) { if (size() > i) data->texs->setValue(t,i); else return 0; return setVert(i,p,n); }
bool VRGeoData::setVert(int i, Pnt3d p, Vec3d n, Vec2d t, Vec2d t2) { if (size() > i) data->texs2->setValue(t2,i); else return 0; return setVert(i,p,n,t); }
bool VRGeoData::setVert(int i, Pnt3d p, Vec3d n, Color3f c, Vec2d t) { if (size() > i) data->texs->setValue(t,i); else return 0; return setVert(i,p,n,c); }
bool VRGeoData::setVert(int i, Pnt3d p, Vec3d n, Color4f c, Vec2d t) { if (size() > i) data->texs->setValue(t,i); else return 0; return setVert(i,p,n,c); }
bool VRGeoData::setVert(int i, Pnt3d p, Vec3d n, Color3f c, Vec2d t, Vec2d t2) { if (size() > i) data->texs2->setValue(t2,i); else return 0; return setVert(i,p,n,c,t); }
bool VRGeoData::setVert(int i, Pnt3d p, Vec3d n, Color4f c, Vec2d t, Vec2d t2) { if (size() > i) data->texs2->setValue(t2,i); else return 0; return setVert(i,p,n,c,t); }

bool VRGeoData::setType(int i, int t) { if (i < (int)data->types->size()) data->types->setValue(t,i); else return 0; return 1; }
bool VRGeoData::setLength(int i, int l) { if (i < (int)data->lengths->size()) data->lengths->setValue(l,i); else return 0; return 1; }
bool VRGeoData::setIndex(int i, int I) { if (i < (int)data->indices->size()) data->indices->setValue(I,i); else return 0; return 1; }
bool VRGeoData::setPos(int i, Pnt3d p) { if (i < (int)data->pos->size()) data->pos->setValue(p,i); else return 0; return 1; }
bool VRGeoData::setNorm(int i, Vec3d n) { if (i < (int)data->norms->size()) data->norms->setValue(n,i); else return 0; return 1; }
bool VRGeoData::setTexCoord(int i, Vec2d t) { if (i < (int)data->texs->size()) data->texs->setValue(t,i); else return 0; return 1; }
bool VRGeoData::setTexCoord2(int i, Vec2d t) { if (i < (int)data->texs2->size()) data->texs2->setValue(t,i); else return 0; return 1; }
bool VRGeoData::setColor(int i, Color3f c) { if (i < (int)data->cols3->size()) data->cols3->setValue(c,i); else return 0; return 1; }
bool VRGeoData::setColor(int i, Color4f c) { if (i < (int)data->cols4->size()) data->cols4->setValue(c,i); else return 0; return 1; }

void VRGeoData::pushQuad(Vec3d p, Vec3d n, Vec3d u, Vec2d s, bool addInds) {
    Vec3d x = -n.cross(u); x.normalize();
    pushVert(p - x*s[0]*0.5 - u*s[1]*0.5, n, Vec2d(0,0));
    pushVert(p + x*s[0]*0.5 - u*s[1]*0.5, n, Vec2d(1,0));
    pushVert(p + x*s[0]*0.5 + u*s[1]*0.5, n, Vec2d(1,1));
    pushVert(p - x*s[0]*0.5 + u*s[1]*0.5, n, Vec2d(0,1));
    if (addInds) pushQuad();
}

int VRGeoData::pushVert(const VRGeoData& other, int i) {
    auto od = other.data;
    if (int(od->pos->size()) <= i) { cout << "VRGeoData::pushVert ERROR: invalid index " << i << endl; return 0; }
    auto p = Pnt3d(od->pos->getValue(i));
    bool doNorms = (od->norms && int(od->norms->size()) > i);
    bool doCol3 = (od->cols3 && int(od->cols3->size()) > i);
    bool doCol4 = (od->cols4 && int(od->cols4->size()) > i);
    bool doTex = (od->texs && int(od->texs->size()) > i);
    bool doTex2 = (od->texs2 && int(od->texs2->size()) > i);
    if (doNorms) {
        auto n = Vec3d(od->norms->getValue(i));
        if (doTex2 && doCol3) return pushVert(p, n, od->cols3->getValue(i), Vec2d(od->texs->getValue(i)), Vec2d(od->texs2->getValue(i)));
        if (doTex2 && doCol4) return pushVert(p, n, od->cols4->getValue(i), Vec2d(od->texs->getValue(i)), Vec2d(od->texs2->getValue(i)));
        if (doTex && doCol3) return pushVert(p, n, od->cols3->getValue(i), Vec2d(od->texs->getValue(i)));
        if (doTex && doCol4) return pushVert(p, n, od->cols4->getValue(i), Vec2d(od->texs->getValue(i)));
        if (doTex2) return pushVert(p, n, Vec2d(od->texs->getValue(i)), Vec2d(od->texs2->getValue(i)));
        if (doTex)  return pushVert(p, n, Vec2d(od->texs->getValue(i)));
        if (doCol3) return pushVert(p, n, od->cols3->getValue(i));
        if (doCol4) return pushVert(p, n, od->cols4->getValue(i));
        return pushVert(p, n);
    }
    return pushVert(p);
}

int VRGeoData::pushVert(const VRGeoData& other, int i, Matrix4d m) {
    auto od = other.data;
    if (int(od->pos->size()) <= i) { cout << "VRGeoData::pushVert ERROR: invalid index " << i << endl; return 0; }
    auto p = Pnt3d(od->pos->getValue(i));
    m.mult(p,p);
    bool doNorms = (od->norms && int(od->norms->size()) > i);
    bool doCol3 = (od->cols3 && int(od->cols3->size()) > i);
    bool doCol4 = (od->cols4 && int(od->cols4->size()) > i);
    bool doTex = (od->texs && int(od->texs->size()) > i);
    bool doTex2 = (od->texs2 && int(od->texs2->size()) > i);
    if (doNorms) {
        Vec3d n = Vec3d(od->norms->getValue(i));
        m.mult(n,n);
        if (doTex2 && doCol3) return pushVert(p, n, od->cols3->getValue(i), Vec2d(od->texs->getValue(i)), Vec2d(od->texs2->getValue(i)));
        if (doTex2 && doCol4) return pushVert(p, n, od->cols4->getValue(i), Vec2d(od->texs->getValue(i)), Vec2d(od->texs2->getValue(i)));
        if (doTex && doCol3) return pushVert(p, n, od->cols3->getValue(i), Vec2d(od->texs->getValue(i)));
        if (doTex && doCol4) return pushVert(p, n, od->cols4->getValue(i), Vec2d(od->texs->getValue(i)));
        if (doTex2) return pushVert(p, n, Vec2d(od->texs->getValue(i)), Vec2d(od->texs2->getValue(i)));
        if (doTex)  return pushVert(p, n, Vec2d(od->texs->getValue(i)));
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
    if (t == GL_POLYGON) return true;
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
#ifdef WASM
    pushTri(i,j,k);
    pushTri(i,k,l);
#else
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
#endif
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

void VRGeoData::pushPrim(Primitive p) {
    int No = primNOffset(p.lid, p.type);
    int N = p.indices.size();
    //int iN0 = size();
    //cout << "pushPrim: " << p.asString() << endl;
    //cout << "pushPrim: " << No << " " << p.lid << " " << size() << endl;
    for (int i = No; i<N; i++) {
        data->indices->addValue( p.indices[i] );
        //cout << "add index: " << p.indices[i] << endl;
    }
    if (p.lid == 0 && isStripOrFan(p.type)) data->lastPrim = -1;
    updateType(p.type, N-No);
}

void VRGeoData::append(const VRGeoData& geo, const Matrix4d& m) {
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

void VRGeoData::append(VRGeometryPtr geo, const Matrix4d& m) { append( VRGeoData(geo), m ); }

VRGeometryPtr VRGeoData::asGeometry(string name) const {
    auto geo = VRGeometry::create(name);
    apply(geo);
    return geo;
}

string VRGeoData::status() {
    string res;
    res += "VRGeoData stats:\n";
    res += " " + toString(data->types->size()) + " types: ";
    for (unsigned int i=0; i<data->types->size(); i++) res += " " + toString(data->types->getValue(i));
    res += "\n";
    res += " " + toString(data->lengths->size()) + " lengths: ";
    for (unsigned int i=0; i<data->lengths->size(); i++) res += " " + toString(data->lengths->getValue(i));
    res += "\n";
    res += " " + toString(data->pos->size()) + " positions\n";
    for (unsigned int i=0; i<data->pos->size(); i++) res += " " + toString(Pnt3d(data->pos->getValue(i)));
    res += "\n";
    res += " " + toString(data->norms->size()) + " normals\n";
    res += " " + toString(data->cols3->size()) + " colors 3\n";
    res += " " + toString(data->cols4->size()) + " colors 4\n";
    res += " " + toString(data->texs->size()) + " texture coordinates\n";
    res += " " + toString(data->texs2->size()) + " texture coordinates 2\n";
    res += " " + toString(data->indices->size()) + " indices: ";
    for (unsigned int i=0; i<data->indices->size(); i++) res += " " + toString(data->indices->getValue(i));
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
    if (type == GL_POLYGON) return -1;

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
    if (type == GL_POLYGON) return 0;

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

    if (Np < 0) { // GL_POLYGON
        Np = l;
    }

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

    for (unsigned int i=0; i<g.data->types->size(); i++) {
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

void VRGeoData::addVertexColors(Color3f c) {
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

void VRGeoData::addVertexColors(Color4f c) {
    int N = size();
    auto& cols = data->cols4;
    if (cols->size() == 0) {
        for (int i=0; i<N; i++) cols->addValue(c);
    }
    if (geo) {
        geo->fixColorMapping();
        geo->getMesh()->geo->setColors(cols);
    }
}

void VRGeoData::makeSingleIndex() {
    if (!geo) return;
    if (geo->getMesh()->geo->isSingleIndex()) return;

    geo->convertToTriangles(); // TODO: temp fix..
    geo->getMesh()->geo->setIndex(geo->getMesh()->geo->getIndex(Geometry::PositionsIndex), Geometry::NormalsIndex);
    geo->getMesh()->geo->setIndex(geo->getMesh()->geo->getIndex(Geometry::PositionsIndex), Geometry::ColorsIndex);

    if (!geo->getMesh()->geo->isSingleIndex()) cout << "VRGeoData::makeSingleIndex FAILED!! probably needs to set more indices!" << endl;
}

vector<VRGeometryPtr> VRGeoData::splitByVertexColors(const Matrix4d& m) {
    map<size_t, VRGeoData> geos;
    map<size_t, Color3f> colors;
    map<size_t, map<size_t, size_t>> indexMaps;

    auto hashColor3 = [&](const Color3f& col) -> size_t {
        size_t h = col[0]*255 + col[1]*255*255 + col[2]*255*255*255;
        colors[h] = col;
        return h;
    };

    auto hashColor4 = [&](const Color4f& col) -> size_t {
        size_t h = col[0]*255 + col[1]*255*255 + col[2]*255*255*255 + col[3]*255*255*255*255;
        colors[h] = Color3f(col[0], col[1], col[2]);
        return h;
    };

    auto getHash = [&](size_t i) -> size_t {
        if (data->cols3) return hashColor3(data->cols3->getValue(i));
        if (data->cols4) return hashColor4(data->cols4->getValue(i));
        return 0;
    };

    for (auto& prim : *this) {
        auto h = getHash(prim.indices[0]);
        auto& geo = geos[h];
        auto& indexMap = indexMaps[h];

        vector<int> ninds;
        for (auto i : prim.indices) {
            if (!indexMap.count(i)) indexMap[i] = geo.pushVert(*this, i);
            ninds.push_back(indexMap[i]);
        }
        prim.indices = ninds;
        geo.pushPrim(prim);
    }

    vector<VRGeometryPtr> res;
    for (auto g : geos) {
        auto gg = g.second.asGeometry(geo?geo->getBaseName():"part");
        gg->setMatrix(m);
        auto mat = VRMaterial::create("part-mat");
        mat->setDiffuse(colors[g.first]);
        gg->setMaterial(mat);
        res.push_back(gg);
    }
    return res;
}







