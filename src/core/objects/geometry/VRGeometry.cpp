#include "VRGeometry.h"
#include "VRGeoData.h"
#include <libxml++/nodes/element.h>

#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.
#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGMultiPassMaterial.h>
#include <OpenSG/OSGGeoFunctions.h>
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGFaceIterator.h>
#include <OpenSG/OSGTypedGeoIntegralProperty.h>
#include <OpenSG/OSGTypedGeoVectorProperty.h>

#include <OpenSG/OSGTriangleIterator.h>
#include "core/scene/import/VRImport.h"
#include "core/math/interpolator.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRGlobals.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/OSGMaterial.h"
#include "core/objects/object/VRObjectT.h"
#include "core/objects/OSGObject.h"
#include "core/tools/selection/VRSelection.h"
#include "core/networking/VRSharedMemory.h"
#include "VRPrimitive.h"
#include "OSGGeometry.h"

#include <OpenSG/OSGIntersectAction.h>
#include <OpenSG/OSGLineIterator.h>
#include <OpenSG/OSGSimpleAttachment.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

map<Geometry*, VRGeometry*> geoAttachmentMap;

const VRGeometryPtr getGeometryAttachment(Geometry* g) {
    VRGeometryPtr z;
    return geoAttachmentMap.count(g) ? geoAttachmentMap[g]->ptr() : z;
}

void setGeometryAttachment(Geometry* g, VRGeometry* geo) {
    geoAttachmentMap[g] = geo;
}

VRGeometry::Reference::Reference(int t, string p) {
    type = t;
    parameter = p;
}

VRObjectPtr VRGeometry::copy(vector<VRObjectPtr> children) {
    VRGeometryPtr geo = VRGeometry::create(getBaseName());
    geo->setMesh(mesh);
    geo->setMaterial(mat);
    geo->source = source;
    geo->setVisible(isVisible());
    geo->setPickable(isPickable());
    geo->setMatrix(getMatrix());
    return geo;
}

class geoIntersectionProxy : public Geometry {
    public:
        bool intersectVolume(IntersectAction* ia) {
            ia->getActNode()->updateVolume();
            const BoxVolume& bv = ia->getActNode()->getVolume();
            if (bv.isValid() && !bv.intersect(ia->getLine())) return false;
            return true;
        }

        bool intersectQuadPatch(IntersectAction* ia) {
            UInt32 numTris = 0;
            Real32 t;
            Vec3f norm;
            const Line& ia_line = ia->getLine();

            auto inds = getIndices();
            auto pos = getPositions();
            for (uint i=0; i<inds->size(); i+=4) { // each 4 indices are a quad
                int i1 = inds->getValue(i+0);
                int i2 = inds->getValue(i+1);
                int i3 = inds->getValue(i+2);
                int i4 = inds->getValue(i+3);

                Pnt3f p1 = pos->getValue<Pnt3f>(i1);
                Pnt3f p2 = pos->getValue<Pnt3f>(i2);
                Pnt3f p3 = pos->getValue<Pnt3f>(i3);
                Pnt3f p4 = pos->getValue<Pnt3f>(i4);

                numTris += 2;
                if (ia_line.intersect(p1, p2, p3, t, &norm)) {
                    ia->setHit(t, ia->getActNode(), i/4, norm, -1);
                }
                if (ia_line.intersect(p1, p3, p4, t, &norm)) {
                    ia->setHit(t, ia->getActNode(), i/4+1, norm, -1);
                }
            }

            ia->getStatCollector()->getElem(IntersectAction::statNTriangles)->add(numTris);
            return ia->didHit();
        }

        Action::ResultE intersectDefaultGeometry(Action* action) {
            if (!getTypes()) return Action::Skip;
            auto type = getTypes()->getValue(0);
            if ( type != GL_PATCHES ) return Geometry::intersectEnter(action);

            if ( getPatchVertices() != 4 ) {
                cout << "Warning: patch vertices is " + toString(getPatchVertices()) + ", not 4, skipping intersect action!\n";
                return Action::Skip;
            }

            IntersectAction* ia = dynamic_cast<IntersectAction*>(action);
            if (!intersectVolume(ia)) return Action::Skip; //bv missed -> can not hit children

            intersectQuadPatch(ia);
            //return Action::Skip;
            return Action::Continue;
        }

        Action::ResultE intersectEnter(Action* action) {
            auto vrGeo = getGeometryAttachment(this);
            if (vrGeo) return vrGeo->applyIntersectionAction(action) ? Action::Continue : Action::Skip;
            return intersectDefaultGeometry(action);
        }
};

bool VRGeometry::applyIntersectionAction(Action* action) {
    if (!mesh || !mesh->geo) return false;
    auto proxy = (geoIntersectionProxy*)mesh->geo.get();
    if (!proxy) return false;
    return proxy->intersectDefaultGeometry(action) == Action::Continue;
}

/** initialise a geometry object with his name **/
VRGeometry::VRGeometry(string name) : VRTransform(name) {
    type = "Geometry";
    addAttachment("geometry", 0);

    store("sourcetype", &source.type);
    store("sourceparam", &source.parameter);

    regStorageSetupFkt( VRFunction<int>::create("geometry_update", boost::bind(&VRGeometry::setup, this)) );

    // override intersect action callbacks for geometry
    IntersectAction::registerEnterDefault( Geometry::getClassType(), reinterpret_cast<Action::Callback>(&geoIntersectionProxy::intersectEnter));
}

VRGeometry::VRGeometry(string name, bool hidden) : VRTransform(name) {
    setNameSpace("system");
    type = "Geometry";
    addAttachment("geometry", 0);
    if (hidden) setPersistency(0);
}

VRGeometry::~VRGeometry() {}

VRGeometryPtr VRGeometry::create(string name) { auto g = VRGeometryPtr(new VRGeometry(name) ); g->setMesh(); return g; }
VRGeometryPtr VRGeometry::create(string name, bool hidden) { auto g = VRGeometryPtr(new VRGeometry(name, hidden) ); g->setMesh(); return g; }
VRGeometryPtr VRGeometry::create(string name, string primitive, string params) {
    auto g = VRGeometryPtr(new VRGeometry(name) );
    g->setPrimitive(primitive, params);
    return g;
}

VRGeometryPtr VRGeometry::ptr() { return static_pointer_cast<VRGeometry>( shared_from_this() ); }

/** Set the geometry mesh (OSG geometry core) **/
void VRGeometry::setMesh(OSGGeometryPtr g, Reference ref, bool keep_material) {
    if (g->geo == 0) return;
    if (mesh_node && mesh_node->node && getNode() && getNode()->node) getNode()->node->subChild(mesh_node->node);

    setGeometryAttachment(g->geo, this);
    mesh = g;
    mesh_node = OSGObject::create( makeNodeFor(g->geo) );
    OSG::setName(mesh_node->node, getName());
    getNode()->node->addChild(mesh_node->node);
    meshSet = true;
    source = ref;

    if (mat == 0) mat = VRMaterial::getDefault();
    if (keep_material) mat = VRMaterial::get(g->geo->getMaterial());
    setMaterial(mat);
    meshChanged();
}

void VRGeometry::setMesh(OSGGeometryPtr g) {
    if (!g) g = OSGGeometry::create( Geometry::create() );
    Reference ref;
    ref.type = CODE;
    setMesh(g, ref);
}

void VRGeometry::meshChanged() { lastMeshChange = VRGlobals::CURRENT_FRAME; }

void VRGeometry::setPrimitive(string primitive, string args) {
    this->primitive = VRPrimitive::make(primitive);
    if (this->primitive == 0) return;
    if (args != "") this->primitive->fromString(args);
    source.type = PRIMITIVE;
    source.parameter = primitive + " " + this->primitive->toString();
    setMesh( OSGGeometry::create( this->primitive->make() ), source);
}

/** Create a mesh using vectors with positions, normals, indices && optionaly texture coordinates **/
void VRGeometry::create(int type, vector<Vec3f> pos, vector<Vec3f> norms, vector<int> inds, vector<Vec2f> texs) {
    bool doTex = (texs.size() == pos.size());

    GeoUInt8PropertyRecPtr      Type = GeoUInt8Property::create();
    GeoUInt32PropertyRecPtr     Length = GeoUInt32Property::create();
    GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr      Norms = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr     Indices = GeoUInt32Property::create();
    SimpleMaterialRecPtr        Mat = SimpleMaterial::create();
    GeoVec2fPropertyRecPtr      Tex = 0;
    if (doTex) Tex = GeoVec2fProperty::create();


    Type->addValue(type);
    Length->addValue(inds.size());

    //positionen und Normalen
    for(uint i=0;i<pos.size();i++) {
            Pos->addValue(pos[i]);
            Norms->addValue(norms[i]);
            if (doTex) Tex->addValue(texs[i]);
    }

    for(uint i=0;i<inds.size();i++) {
            Indices->addValue(inds[i]);
    }

    Mat->setDiffuse(Color3f(0.8,0.8,0.6));
    Mat->setAmbient(Color3f(0.4, 0.4, 0.2));
    Mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    GeometryRecPtr geo = Geometry::create();
    geo->setTypes(Type);
    geo->setLengths(Length);
    geo->setIndices(Indices);
    geo->setPositions(Pos);
    geo->setNormals(Norms);
    if (doTex) geo->setTexCoords(Tex);
    geo->setMaterial(Mat);

    setMesh( OSGGeometry::create(geo) );
}

/** Create a mesh using vectors with positions, normals, indices && optionaly texture coordinates **/
void VRGeometry::create(int type, GeoVectorProperty* pos, GeoVectorProperty* norms, GeoIntegralProperty* inds, GeoVectorProperty* texs) {
    setType(type);
    setPositions(pos);
    setNormals(norms);
    setIndices(inds);
    setTexCoords(texs);
}

/** Overwrites the vertex positions of the mesh **/
void VRGeometry::setPositions(GeoVectorProperty* Pos) {
    if (!meshSet) setMesh();
    //if (Pos->size() == 1) Pos->addValue(Pnt3f()); // hack to avoid the single point bug
    mesh->geo->setPositions(Pos);
    meshChanged();
}

void VRGeometry::setType(int t) {
    if (!meshSet) setMesh();
    GeoUInt8PropertyRecPtr Type = GeoUInt8Property::create();
    Type->addValue(t);
    setTypes(Type);
}

void VRGeometry::makeUnique() {
    if (mesh_node == 0) return;
    NodeMTRecPtr clone = deepCloneTree( mesh_node->node );
    setMesh( OSGGeometry::create( dynamic_cast<Geometry*>( clone->getCore() ) ), source );
}

void VRGeometry::fixColorMapping() {
    mesh->geo->setIndex(mesh->geo->getIndex(Geometry::PositionsIndex), Geometry::ColorsIndex);
}

// OSG 2.0 function not implemented :(
// this is a port of the function in OSG 1.8
void calcFaceNormals(GeometryMTRecPtr geo) {
    if (!geo->getPositions() || geo->getPositions()->size() == 0) {
        cout << "Warning: no positions for calcFaceNormals\n";
        return;
    }

    GeoUInt32PropertyRecPtr newIndex = GeoUInt32Property::create();
    GeoVec3fPropertyRecPtr newNormals = GeoVec3fProperty::create();
    Vec3f normal;

    FaceIterator faceIter = geo->beginFaces();
    GeoIntegralPropertyRecPtr oldPosIndex = geo->getIndex(Geometry::PositionsIndex);
    GeoIntegralPropertyRecPtr oldNormsIndex = geo->getIndex(Geometry::NormalsIndex);

    auto calcNormal = [&](FaceIterator& f) {
        Vec3f normal;
        if (f.getLength() == 3) { // Face is a triangle
            normal = ( f.getPosition(1) - f.getPosition(0) ).cross(f.getPosition(2) - f.getPosition(0));
        } else { // Face must be a quad
            normal = ( f.getPosition(1) - f.getPosition(0) ).cross(f.getPosition(2) - f.getPosition(0));
            if (normal.length() == 0) { // Quad is degenerate, choose different points for normal
                normal = ( f.getPosition(1) - f.getPosition(2) ).cross(f.getPosition(3) - f.getPosition(2));
            }
        }
        normal.normalize();
        return normal;
    };

    /*auto calcIndices = [&](FaceIterator& f) {
        vector<int> res;
        UInt32 base;
        switch(f.getType()) {
            case GL_TRIANGLE_FAN:
            case GL_TRIANGLE_STRIP:
                base = f.getIndex(2);                   // get last point's position in index field
                res.push_back(base + (base / oldIMSize) + oldIMSize);
                break;
            case GL_QUAD_STRIP:
                base = f.getIndex(3);                   // get last point's position in index field
                res.push_back(base + (base / oldIMSize) + oldIMSize);
                break;
            default:
                for(UInt32 i = 0; i < f.getLength(); ++i) {
                    base = f.getIndex(i);
                    res.push_back(base + (base / oldIMSize) + oldIMSize);
                }
                break;
        }
        return res;
    };*/

    if (oldPosIndex) { //Indexed
        /*if (oldPosIndex != oldNormsIndex) { // multi indexed -> TODO
            MFUInt16& oldIndexMap = geo->getIndexMapping();
            UInt32 oldIMSize = oldIndexMap.size();

            for (UInt32 i = 0; i < oldIndex->size() / oldIMSize; ++i) {
                for (UInt32 k = 0; k < oldIMSize; ++k) newIndex->push_back(oldIndex->getValue(i * oldIMSize + k));
                newIndex->push_back(0); //placeholder for normal index
            }

            for (UInt32 faceCnt = 0; faceIter != geo->endFaces(); ++faceIter, ++faceCnt) {
                normal = calcNormal(faceIter);
                newNormals->push_back(normal);
                for (auto i : calcIndices(faceIter)) newIndex->setValue(faceCnt, i);
            }

            Int16 ni = geo->calcMappingIndex(Geometry::MapNormal);
            if (ni != -1) oldIndexMap[ni] = oldIndexMap[ni] &~Geometry::MapNormal;
            oldIndexMap.push_back(Geometry::MapNormal);
            geo->setNormals(newNormals);
            geo->setIndices(newIndex);
            return;
        }*/
    }

    newIndex->resize(oldPosIndex->size());
    for(; faceIter != geo->endFaces(); ++faceIter) {
        normal = calcNormal(faceIter);
        newNormals->addValue(normal);
        int nIndex = newNormals->size()-1;

        switch(faceIter.getType()) {
            case GL_TRIANGLE_FAN:
            case GL_TRIANGLE_STRIP:
                newIndex->setValue(nIndex, faceIter.getIndex(2));
                break;
            case GL_QUAD_STRIP:
                newIndex->setValue(nIndex, faceIter.getIndex(3));
                break;
            default:
                for (UInt32 i = 0; i < faceIter.getLength(); ++i) {
                    newIndex->setValue(nIndex, faceIter.getIndex(i));
                }
                break;
            }
    }

    geo->setNormals(newNormals);
    geo->setIndex(newIndex, Geometry::NormalsIndex);
}

void VRGeometry::updateNormals(bool face) {
    if (!meshSet) return;
    if (face) calcFaceNormals(mesh->geo);
    else calcVertexNormals(mesh->geo);
}

int VRGeometry::getLastMeshChange() { return lastMeshChange; }

void VRGeometry::setTypes(GeoIntegralProperty* types) { if (!meshSet) setMesh(); mesh->geo->setTypes(types); }
void VRGeometry::setNormals(GeoVectorProperty* Norms) { if (!meshSet) setMesh(); mesh->geo->setNormals(Norms); }
void VRGeometry::setColors(GeoVectorProperty* Colors, bool fixMapping) { if (!meshSet) setMesh(); mesh->geo->setColors(Colors); if (fixMapping) fixColorMapping(); }
void VRGeometry::setLengths(GeoIntegralProperty* lengths) { if (!meshSet) setMesh(); mesh->geo->setLengths(lengths); }
void VRGeometry::setTexCoords(GeoVectorProperty* Tex, int i, bool fixMapping) {
    if (!meshSet) setMesh();
    if (i == 0) mesh->geo->setTexCoords(Tex);
    if (i == 1) mesh->geo->setTexCoords1(Tex);
    if (i == 2) mesh->geo->setTexCoords2(Tex);
    if (i == 3) mesh->geo->setTexCoords3(Tex);
    if (i == 4) mesh->geo->setTexCoords4(Tex);
    if (i == 5) mesh->geo->setTexCoords5(Tex);
    if (i == 6) mesh->geo->setTexCoords6(Tex);
    if (i == 7) mesh->geo->setTexCoords7(Tex);
    if (fixMapping) mesh->geo->setIndex(mesh->geo->getIndex(Geometry::PositionsIndex), Geometry::TexCoordsIndex);
}

void VRGeometry::setPositionalTexCoords(float scale, int i, Vec3i format) {
    GeoVectorPropertyRefPtr pos = mesh->geo->getPositions();
    GeoVec3fPropertyRefPtr tex = GeoVec3fProperty::create();
    for (uint i=0; i<pos->size(); i++) {
        auto p = Vec3f(pos->getValue<Pnt3f>(i))*scale;
        tex->addValue(Vec3f(p[format[0]], p[format[1]], p[format[2]]));
    }
    setTexCoords(tex, i, 1);
}

void VRGeometry::setPositionalTexCoords2D(float scale, int i, Vec2i format) {
    GeoVectorPropertyRefPtr pos = mesh->geo->getPositions();
    GeoVec3fPropertyRefPtr tex = GeoVec3fProperty::create();
    for (uint i=0; i<pos->size(); i++) {
        auto p = Vec3f(pos->getValue<Pnt3f>(i))*scale;
        tex->addValue(Vec2f(p[format[0]], p[format[1]]));
    }
    setTexCoords(tex, i, 1);
}

void VRGeometry::setIndices(GeoIntegralProperty* Indices, bool doLengths) {
    if (!meshSet) setMesh();
    if (Indices->size() == 0) setMesh(0);
    if (!mesh->geo->getLengths() || doLengths) {
        GeoUInt32PropertyRecPtr Length = GeoUInt32Property::create();
        Length->addValue(Indices->size());
        mesh->geo->setLengths(Length);
    }
    mesh->geo->setIndices(Indices);
}

int VRGeometry::size() {
    auto m = getMesh();
    auto p = m->geo->getPositions();
    return p->size();
}

int getColorChannels(GeoVectorProperty* v) {
    if (v == 0) return 0;
    int type = v->getType().getId();
    if (type == 1749) return 3;
    if (type == 1775) return 3;
    if (type == 1776) return 4;
    cout << "getColorChannels WARNING: unknown type ID " << type << endl;
    return 0;
}

Vec3f morphColor3(const Vec3f& c) { return c; }
Vec3f morphColor3(const Vec4f& c) { return Vec3f(c[0], c[1], c[2]); }
Vec4f morphColor4(const Vec3f& c) { return Vec4f(c[0], c[1], c[2], 1); }
Vec4f morphColor4(const Vec4f& c) { return c; }

void VRGeometry::merge(VRGeometryPtr geo) {
    if (!geo) return;
    if (!geo->mesh->geo) return;
    if (!meshSet) setMesh();

    Matrix M = getWorldMatrix();
    M.invert();
    M.mult( geo->getWorldMatrix() );

    VRGeoData self(ptr());
    VRGeoData other(geo);
    self.append(other, M);
    self.apply(ptr());
}

void VRGeometry::removeSelection(VRSelectionPtr sel) {
    if (!mesh->geo) return;

    VRGeoData newData;
    VRGeoData self(ptr());

    map<int, int> mapping;
    auto addVertex = [&](int i, bool mapit = true) {
        int j = newData.size();
        if (mapit) mapping[i] = j;
        newData.pushVert(self, i);
        return j;
    };

    // copy not selected vertices
    auto sinds = sel->getSubselection(ptr());
    std::sort(sinds.begin(), sinds.end());
    std::unique(sinds.begin(), sinds.end());
    for (uint k=0, i=0; i < uint(self.size()); i++) {
        bool selected = false;
        if (k < sinds.size()) if (int(i) == sinds[k]) selected = true;
        if (!selected) addVertex(i);
        else k++;
    }

    // copy not selected and partially selected triangles
    for (auto p : self) {
        bool all = true;
        bool any = false;
        for (int i : p.indices) {
            if ( mapping.count(i) ) any = true;
            else all = false;
        }

        vector<int> ninds;
        for (int i : p.indices) {
            if ( mapping.count(i) ) ninds.push_back( mapping[i] );
            else ninds.push_back( addVertex(i, false) );
        }
        p.indices = ninds;
        if (all || any) newData.pushPrim(p);
    }

    newData.apply(ptr());
}

VRGeometryPtr VRGeometry::copySelection(VRSelectionPtr sel) {
    VRGeoData self(ptr());
    VRGeoData selData;

    // copy selected vertices
    auto sinds = sel->getSubselection(ptr());
    std::sort(sinds.begin(), sinds.end());
    std::unique(sinds.begin(), sinds.end());
    map<int, int> mapping;
    int k = 0;
    for (int i : sinds) {
        selData.pushVert( self, i );
        mapping[i] = k;
        k++;
    }

    // copy selected primitives
    for (auto& p : self) {
        bool mapped = true;
        for (int i : p.indices) if (!mapping.count(i)) { mapped = false; break; }
        if (!mapped) continue;

        vector<int> ninds;
        for (int i : p.indices) ninds.push_back( mapping[i] );
        p.indices = ninds;

        selData.pushPrim(p);
    }

    return selData.asGeometry(getName());
}

VRGeometryPtr VRGeometry::separateSelection(VRSelectionPtr sel) {
    auto geo = copySelection(sel);
    removeSelection(sel);
    return geo;
}

void VRGeometry::genTexCoords(string mapping, float scale, int channel, shared_ptr<pose> uvp) {
    GeoVec2fPropertyRecPtr tex = GeoVec2fProperty::create();
    Matrix uvp_inv;
    if (uvp) uvp_inv = uvp->asMatrix();
    if (uvp) uvp_inv.invert();

    auto pos = mesh->geo->getPositions();
    auto norms = mesh->geo->getNormals();
    int N = pos->size();

    Vec3f p;
    Vec3f n;
    Vec2f uv;

    auto cubeMapping = [&](Vec3f& p, Vec3f& n, Vec2f& uv) {
        if (abs(n[0]) > abs(n[1]) && abs(n[0]) > abs(n[2])) uv = Vec2f(p[1], p[2]);
        if (abs(n[1]) > abs(n[0]) && abs(n[1]) > abs(n[2])) uv = Vec2f(p[0], p[2]);
        if (abs(n[2]) > abs(n[0]) && abs(n[2]) > abs(n[1])) uv = Vec2f(p[0], p[1]);
    };

    auto sphereMapping = [&](Vec3f& p, Vec3f& n, Vec2f& uv) {
        float r = p.length();
        float theta = acos(p[1]/r);
        float phi   = atan(p[2]/p[0]);
        uv = Vec2f(abs(phi),abs(theta));
    };

    for (int i=0; i<N; i++) {
        Vec3f n = norms->getValue<Vec3f>(i);
        Vec3f p = Vec3f(pos->getValue<Pnt3f>(i));
        if (uvp) uvp_inv.mult( p,p );
        if (uvp) uvp_inv.mult( n,n );
        if (mapping == "CUBE") cubeMapping(p,n,uv);
        if (mapping == "SPHERE") sphereMapping(p,n,uv);
        tex->addValue(uv*scale);
    }

    setTexCoords(tex, channel, true);
}

void VRGeometry::decimate(float f) {
    /*if (mesh == 0) return;

    map<int, int> collapsing;
    map<int, Pnt3f> collapse_points;
    map<int, Vec3f> collapse_normals;
	TriangleIterator it(mesh);
	for(int i=0; !it.isAtEnd(); ++it, i++) {
        float r = (float)rand()/RAND_MAX;
        if (r <= f) continue;
        Vec3i in(it.getPositionIndex(0), it.getPositionIndex(1), it.getPositionIndex(2));

        if (collapsing.count(in[0])) continue;
        if (collapsing.count(in[1])) continue;

        collapsing[in[0]] = in[1];
        collapsing[in[1]] = in[0];

        //Pnt3f p = (it.getPosition(0) + Vec3f(it.getPosition(1)))*0.5; // edge midpoint
        //Vec3f n = (it.getNormal(0) + it.getNormal(1))*0.5;

        Pnt3f p = it.getPosition(0);
        Vec3f n = it.getNormal(0);

        collapse_points[in[0]] = p;
        collapse_points[in[1]] = p;
        collapse_normals[in[0]] = n;
        collapse_normals[in[1]] = n;
	}

	GeoPnt3fPropertyRecPtr positions = GeoPnt3fProperty::create();
	GeoUInt32PropertyRecPtr indices = GeoUInt32Property::create();
	GeoVec3fPropertyRecPtr normals = GeoVec3fProperty::create();

	GeoUInt32PropertyRecPtr idx = dynamic_cast<GeoUInt32Property*>(mesh->getIndices());
	for (uint i=0; i<idx->size(); i++) {
        cout << "   VRGeometry::decimate " << i << " " << idx->getValue(i) << endl;
	}

	TriangleIterator it2(mesh);
	for(int i=0; !it2.isAtEnd(); ++it2) { // simplify mesh
        Vec3i in(it2.getPositionIndex(0), it2.getPositionIndex(1), it2.getPositionIndex(2));
        Vec3b inc(collapsing.count(in[0]), collapsing.count(in[1]), collapsing.count(in[2]));

        //inc = Vec3b(false, false, false);
        cout << "   VRGeometry::decimate " << inc << "   " << in << endl;

        if (inc[0]) { // collapse one edge, no triangle!
            if(collapsing[in[0]] == in[1]) continue;
            if(collapsing[in[0]] == in[2]) continue;
        }

        if (inc[1]) {
            if(collapsing[in[1]] == in[0]) continue;
            if(collapsing[in[1]] == in[2]) continue;
        }

        if (inc[2]) {
            if(collapsing[in[2]] == in[0]) continue;
            if(collapsing[in[2]] == in[1]) continue;
        }

	    for(int j = 0; j < 3; j++) {
            indices->addValue( i*3+j );
            if (!inc[j]) {
                positions->addValue( it2.getPosition(j) );
                normals->addValue( it2.getNormal(j) );
            }
            else {
                positions->addValue( collapse_points[in[j]] );
                normals->addValue( collapse_normals[in[j]] );
                //cout << "   VRGeometry::decimate " << collapse_points[in[j]] << endl;
            }
	    }

        i++;
	}

	cout << "VRGeometry::decimate " << f << " " << positions->size() << endl;
	cout << " VRGeometry::decimate " << collapsing.size() << endl;

	setIndices(indices);
	setPositions(positions);
	setNormals(normals);
	setType(GL_TRIANGLES);

	GeoUInt32PropertyRecPtr lengths = GeoUInt32Property::create();
	lengths->addValue(indices->size());
	setLengths(lengths);


    createSharedIndex(mesh);*/
}

void VRGeometry::removeDoubles(float minAngle) {// TODO: use angle
    createSharedIndex(mesh->geo);
}

void VRGeometry::setRandomColors() {
    if (!mesh->geo) return;
    GeoPnt3fPropertyRecPtr pos = dynamic_cast<GeoPnt3fProperty*>(mesh->geo->getPositions());
    if (!pos) return;
	int N = pos->size();

	GeoVec4fPropertyRecPtr cols = GeoVec4fProperty::create();
	for (int i=0; i<N; i++) {
        Color4f c; c.setRandom();
        cols->addValue( c );
	}
    setColors(cols);
}

/** Returns the geometric center of the mesh **/
Vec3f VRGeometry::getGeometricCenter() {
    if (!meshSet) return Vec3f(0,0,0);
    GeoPnt3fPropertyRecPtr pos = dynamic_cast<GeoPnt3fProperty*>(mesh->geo->getPositions());

    Vec3f center = Vec3f(0,0,0);
    for (uint i=0;i<pos->size();i++)
        center += Vec3f(pos->getValue(i));

    center *= 1./pos->size();

    return center;
}

/** Returns the average of all normals of the mesh (not normalized, can be zero) **/
Vec3f VRGeometry::getAverageNormal() {
    if (!meshSet) return Vec3f(0,1,0);
    GeoVec3fPropertyRecPtr norms = dynamic_cast<GeoVec3fProperty*>(mesh->geo->getNormals());

    Vec3f normal = Vec3f(0,0,0);
    for (uint i=0;i<norms->size();i++) {
        normal += Vec3f(norms->getValue(i));
    }

    normal *= 1./norms->size();

    return normal;
}

void VRGeometry::influence(vector<Vec3f> pnts, vector<Vec3f> values, int power, float color_code, float dl_max) {
    interpolator inp;
    inp.setPoints(pnts);
    inp.setValues(values);
    if (color_code > 0) {
        if (mesh->geo->getColors() == 0) {
            GeoVec4fPropertyRecPtr cols = GeoVec4fProperty::create();
            cols->resize(mesh->geo->getPositions()->size());
            setColors(cols);
            fixColorMapping();
        }
        inp.evalVec(mesh->geo->getPositions(), power, mesh->geo->getColors(), color_code, dl_max);
    }
    else inp.evalVec(mesh->geo->getPositions(), power);
}

void VRGeometry::setPatchVertices(int n) { if (!meshSet) return; mesh->geo->setPatchVertices(n); }

/** Returns the maximum position on the x, y || z axis **/
float VRGeometry::getMax(int axis) {
    if (!meshSet) return 0;
    if (axis != 0 && axis != 1 && axis != 2) return 0;
    GeoPnt3fPropertyRecPtr pos = dynamic_cast<GeoPnt3fProperty*>(mesh->geo->getPositions());

    float max = pos->getValue(0)[axis];
    for (uint i=0;i<pos->size();i++) {
        if (max < pos->getValue(i)[axis]) max = pos->getValue(i)[axis];
    }

    return max;
}

/** Returns the minimum position on the x, y || z axis **/
float VRGeometry::getMin(int axis) {
    if (!meshSet) return 0;
    if (axis != 0 && axis != 1 && axis != 2) return 0;
    GeoPnt3fPropertyRecPtr pos = dynamic_cast<GeoPnt3fProperty*>(mesh->geo->getPositions());

    float min = pos->getValue(0)[axis];
    for (uint i=0;i<pos->size();i++) {
        if (min > pos->getValue(i)[axis]) min = pos->getValue(i)[axis];
    }

    return min;
}

/** Returns the mesh as a OSG geometry core **/
OSGGeometryPtr VRGeometry::getMesh() {
    if(meshSet) return mesh;
    else return 0;
}

VRPrimitive* VRGeometry::getPrimitive() { return primitive; }

void VRGeometry::setMeshVisibility(bool b) {
    if (!mesh_node) return;
    if (b) mesh_node->node->setTravMask(0xffffffff);
    else mesh_node->node->setTravMask(0);
}

/** Set the material of the mesh **/
void VRGeometry::setMaterial(VRMaterialPtr mat) {
    if (mat == 0) mat = this->mat;
    if (mat == 0) return;

    this->mat = mat;
    if (!meshSet) return;
    mesh->geo->setMaterial(mat->getMaterial()->mat);
}

/*void VRGeometry::setMaterial(MaterialRecPtr mat) {
    if (!meshSet) return;
    if (mat == 0) return;

    if (this->mat == 0) this->mat = VRMaterial::create("mat");
    this->mat->setMaterial(mat);

    setMaterial(this->mat);
}*/

VRMaterialPtr VRGeometry::getMaterial() { return mat; }

float VRGeometry::calcSurfaceArea() {
    if (!meshSet) return 0;

    float A = 0;
    TriangleIterator it(mesh->geo);

	for(int i=0; !it.isAtEnd(); ++it, i++) {
        Pnt3f p0 = it.getPosition(0);
        Pnt3f p1 = it.getPosition(1);
        Pnt3f p2 = it.getPosition(2);
        Vec3f d1 = p1-p0;
        Vec3f d2 = p2-p0;
        A += d1.cross(d2).length();
	}

    return 0.5*A;
}

void VRGeometry::applyTransformation(shared_ptr<pose> po) {
    Matrix m = po->asMatrix();
    if (!mesh) return;
    if (!mesh->geo) return;
    auto pos = mesh->geo->getPositions();
    if (!pos) return;
    auto norms = mesh->geo->getNormals();
    Vec3f n; Pnt3f p;

    for (uint i=0; i<pos->size(); i++) {
        p = pos->getValue<Pnt3f>(i);
        m.mult(p,p);
        pos->setValue(p,i);
    };

    for (uint i=0; i<norms->size(); i++) {
        n = norms->getValue<Vec3f>(i);
        m.mult(n,n);
        norms->setValue(n,i);
    };
}

void VRGeometry::applyTransformation() {
    applyTransformation(getPose());
    setMatrix(Matrix());
}

void VRGeometry::setReference(Reference ref) { source = ref; }
VRGeometry::Reference VRGeometry::getReference() { return source; }

void VRGeometry::showGeometricData(string type, bool b) {
    if (dataLayer.count(type)) dataLayer[type]->destroy();

    VRGeometryPtr geo = VRGeometry::create("DATALAYER_"+getName()+"_"+type, true);
    dataLayer[type] = geo;
    addChild(geo);

    GeoColor3fPropertyRecPtr cols = GeoColor3fProperty::create();
    GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
    GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();

    Pnt3f p;
    Vec3f n;

    if (type == "Normals") {
        GeoVectorPropertyRecPtr g_norms = mesh->geo->getNormals();
        GeoVectorPropertyRecPtr g_pos = mesh->geo->getPositions();
        for (uint i=0; i<g_norms->size(); i++) {
            p = g_pos->getValue<Pnt3f>(i);
            n = g_norms->getValue<Vec3f>(i);
            pos->addValue(p);
            pos->addValue(p+n*0.1);
            cols->addValue(Vec3f(1,1,1));
            cols->addValue(Vec3f(abs(n[0]),abs(n[1]),abs(n[2])));
            inds->addValue(2*i);
            inds->addValue(2*i+1);
        }

        geo->setPositions(pos);
        geo->setType(GL_LINE);
        geo->setColors(cols);
        geo->setIndices(inds);
    }

    VRMaterialPtr m = VRMaterial::create("some-mat");
    geo->setMaterial(m);
    m->setLit(false);
}

void VRGeometry::setup() {
    string p1, p2, p3, p4;
    stringstream ss;
    VRGeometryPtr g;
    // get source info
    // construct data from that

    string bname = getBaseName();
    setName( "TMP_GEO_SETUP_NAME" );

    switch(source.type) {
        case CODE:
            return;
        case SCRIPT:
            break;
        case FILE:
            ss << source.parameter;
            ss >> p1; ss >> p2;
            if (!(ss >> p3)) p3 = "OSG";
            if (!(ss >> p4)) p4 = "0";
            //g = VRImport::get()->loadGeometry(p1, p2, p3, toBool(p4)); // TODO: set callback for thread load
            g = VRImport::get()->loadGeometry(p1, p2, p3, false); // TODO: set callback for thread load
            if (g) setMesh( g->getMesh(), source, true );
            else cout << "failed to load " << p2 << " from file " << p1 << endl;
            break;
        case PRIMITIVE:
            ss << source.parameter;
            ss >> p1; getline(ss, p2);
            setPrimitive(p1, p2);
            break;
    }

    setName( bname );
}

void VRGeometry::readSharedMemory(string segment, string object) {
    VRSharedMemory sm(segment, false);

    int sm_state = sm.getObject<int>(object+"_state");
    while (sm.getObject<int>(object+"_state") == sm_state) {
        cout << "VRGeometry::readSharedMemory: waiting for data: " << sm_state << endl;
        sleep(1);
    }

    // read buffer
    auto sm_types = sm.getVector<int>(object+"_types");
    auto sm_lengths = sm.getVector<int>(object+"_lengths");
    auto sm_pos = sm.getVector<float>(object+"_pos");
    auto sm_norms = sm.getVector<float>(object+"_norms");
    auto sm_inds = sm.getVector<int>(object+"_inds");
    auto sm_cols = sm.getVector<float>(object+"_cols");

    GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr norms = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();
    GeoUInt32PropertyRecPtr types = GeoUInt32Property::create();
    GeoUInt32PropertyRecPtr lengths = GeoUInt32Property::create();
    GeoVec4fPropertyRecPtr cols = GeoVec4fProperty::create();

    cout << "SM mesh read: " << sm_types.size() << " " << sm_lengths.size() << " " << sm_pos.size() << " " << sm_norms.size() << " " << sm_inds.size() << " " << sm_cols.size() << endl;

    if (sm_types.size() > 0) for (auto& t : sm_types) types->addValue(t);
    if (sm_lengths.size() > 0) for (auto& l : sm_lengths) lengths->addValue(l);
    for (auto& i : sm_inds) inds->addValue(i);
    if (sm_pos.size() > 0) for (uint i=0; i<sm_pos.size()-2; i+=3) pos->addValue(Pnt3f(sm_pos[i], sm_pos[i+1], sm_pos[i+2]));
    if (sm_norms.size() > 0) for (uint i=0; i<sm_norms.size()-2; i+=3) norms->addValue(Vec3f(sm_norms[i], sm_norms[i+1], sm_norms[i+2]));
    if (sm_cols.size() > 0) for (uint i=0; i<sm_cols.size()-2; i+=3) cols->addValue(Pnt3f(sm_cols[i], sm_cols[i+1], sm_cols[i+2]));

    cout << "osg mesh data: " << types->size() << " " << lengths->size() << " " << pos->size() << " " << norms->size() << " " << inds->size() << " " << cols->size() << endl;

    uint N = pos->size();
    if (N == 0) return;

    setTypes(types);
    setLengths(lengths);
    setPositions(pos);
    if (norms->size() == N) setNormals(norms);
    if (cols->size() == N) setColors(cols);
    setIndices(inds);
}



OSG_END_NAMESPACE;
