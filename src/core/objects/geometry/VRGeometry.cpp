#include "VRGeometry.h"
#include "VRGeoData.h"
#include <sstream>
#include <thread>

#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.
#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGMultiPassMaterial.h>
#include <OpenSG/OSGGeoFunctions.h>
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGFaceIterator.h>
#include <OpenSG/OSGTypedGeoIntegralProperty.h>
#include <OpenSG/OSGTypedGeoVectorProperty.h>
#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGNameAttachment.h>

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
#include "core/objects/VRPointCloud.h"
#include "core/math/Octree.h"
#include "core/tools/selection/VRSelection.h"
#ifndef WITHOUT_SHARED_MEMORY
#include "core/networking/VRSharedMemory.h"
#endif
#include "VRPrimitive.h"
#include "OSGGeometry.h"

#include <OpenSG/OSGIntersectAction.h>
#include <OpenSG/OSGLineIterator.h>
#include <OpenSG/OSGSimpleAttachment.h>
#include <OpenSG/OSGGroup.h>
#include <OpenSG/OSGTransform.h>

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

void remGeometryAttachment(Geometry* g) {
    if (g) if (geoAttachmentMap.count(g)) geoAttachmentMap.erase(g);
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
    geo->setEntity(entity);
    geo->setMatrix(getMatrix());
    return geo;
}

void applyVolumeCheck2(NodeMTRecPtr n, bool b) {
    if (!n) return;
    BoxVolume &vol = n->editVolume(false);
    vol.setInfinite(!b);
    vol.setStatic(!b);
    vol.setValid(!b);
}

// applies also to mesh node if present
void VRGeometry::setVolumeCheck(bool b, bool recursive) {
    if (!getNode()) return;
    applyVolumeCheck2(getNode()->node, b);
    if (mesh_node) applyVolumeCheck2(mesh_node->node, b);
    if (recursive) VRObject::setVolumeCheck(b,recursive);
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
            for (unsigned int i=0; i<inds->size(); i+=4) { // each 4 indices are a quad
                int i1 = inds->getValue(i+0);
                int i2 = inds->getValue(i+1);
                int i3 = inds->getValue(i+2);
                int i4 = inds->getValue(i+3);

                auto p1 = pos->getValue<Pnt3f>(i1);
                auto p2 = pos->getValue<Pnt3f>(i2);
                auto p3 = pos->getValue<Pnt3f>(i3);
                auto p4 = pos->getValue<Pnt3f>(i4);

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

class groupIntersectionProxy : public Group {
    public:
        Action::ResultE intersectEnter(Action* action) {
            auto ia = dynamic_cast<VRIntersectAction*>(action);
            if (ia->skipVolume()) return Action::Continue;
            return Group::intersectEnter(action);
        }
};

class transIntersectionProxy : public Transform {
    public:
        Action::ResultE intersectEnter(Action* action) {
            auto ia = dynamic_cast<VRIntersectAction*>(action);
            if (ia->skipVolume()) { // use code from Transform::intersectAction
                IntersectAction *ia = dynamic_cast<IntersectAction *>(action);
                Matrix m  = this->getMatrix();
                m.invert();
                Pnt3f pos;
                Vec3f dir;
                m.multFull(ia->getLine().getPosition (), pos);
                m.mult    (ia->getLine().getDirection(), dir);
                Real32 length = dir.length();
                ia->setLine(Line(pos, dir), ia->getMaxDist());
                ia->scale(length);
                return Action::Continue;
            }
            return Transform::intersectEnter(action);
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
    addTag("geometry");

    store("sourcetype", &source.type);
    store("sourceparam", &source.parameter);

    regStorageSetupFkt( VRStorageCb::create("geometry_update", bind(&VRGeometry::setup, this, _1)) );

    // override intersect action callbacks for geometry
    IntersectAction::registerEnterDefault( Group::getClassType(), reinterpret_cast<Action::Callback>(&groupIntersectionProxy::intersectEnter));
    IntersectAction::registerEnterDefault( Transform::getClassType(), reinterpret_cast<Action::Callback>(&transIntersectionProxy::intersectEnter));
    IntersectAction::registerEnterDefault( Geometry::getClassType(), reinterpret_cast<Action::Callback>(&geoIntersectionProxy::intersectEnter));
}

VRGeometry::VRGeometry(string name, bool hidden) : VRTransform(name) {
    setNameSpace("system");
    type = "Geometry";
    addTag("geometry");
    if (hidden) setPersistency(0);
}

VRGeometry::~VRGeometry() {
    if (mesh) remGeometryAttachment(mesh->geo);
}

VRGeometryPtr VRGeometry::create(string name) { auto g = VRGeometryPtr(new VRGeometry(name) ); g->setMesh(); return g; }
VRGeometryPtr VRGeometry::create(string name, bool hidden) { auto g = VRGeometryPtr(new VRGeometry(name, hidden) ); g->setMesh(); return g; }
VRGeometryPtr VRGeometry::create(string name, string primitive, string params) {
    auto g = VRGeometryPtr(new VRGeometry(name) );
    g->setPrimitive(primitive + " " + params);
    return g;
}

VRGeometryPtr VRGeometry::ptr() { return static_pointer_cast<VRGeometry>( shared_from_this() ); }

void VRGeometry::wrapOSG(OSGObjectPtr node, OSGObjectPtr geoNode) {
    VRTransform::wrapOSG(node);
    mesh_node = geoNode;
    Geometry* geo = dynamic_cast<Geometry*>(geoNode->node->getCore());
    mesh = OSGGeometry::create(geo);
    setGeometryAttachment(geo, this);
    meshSet = true;
    source.type = CODE;
    mat = VRMaterial::get(geo->getMaterial());
    setMaterial(mat);
}

/** Set the geometry mesh (OSG geometry core) **/
void VRGeometry::setMesh(OSGGeometryPtr geo, Reference ref, bool keep_material) {
    if (geo->geo == 0) return;
    if (mesh) remGeometryAttachment(mesh->geo);
    if (mesh_node && mesh_node->node && getNode() && getNode()->node) getNode()->node->subChild(mesh_node->node);

    SFUnrecChildGeoIntegralPropertyPtr* types = geo->geo->editSFTypes();
    SFUnrecChildGeoIntegralPropertyPtr* lengths = geo->geo->editSFLengths();
    MFUnrecChildGeoVectorPropertyPtr* props = geo->geo->editMFProperties();
    MFUnrecChildGeoIntegralPropertyPtr* inds = geo->geo->editMFPropIndices();

    int i=0;
    if (types->getValue()) { geo->geo->addAttachment(types->getValue(), i); i++; }
    if (lengths->getValue()) { geo->geo->addAttachment(lengths->getValue(), i); i++; }
    for (auto prop : *inds) if (prop) { geo->geo->addAttachment(prop, i); i++; }
    for (auto prop : *props) if (prop) { geo->geo->addAttachment(prop, i); i++; }

    setGeometryAttachment(geo->geo, this);
    mesh = geo;
    mesh_node = OSGObject::create( makeNodeFor(geo->geo) );
    OSG::setName(mesh_node->node, getName());
    getNode()->node->addChild(mesh_node->node);
    meshSet = true;
    source = ref;

    if (mat == 0) mat = VRMaterial::getDefault();
    if (keep_material) mat = VRMaterial::get(geo->geo->getMaterial());
    setMaterial(mat);
    meshChanged();

#ifdef WASM
    makeSingleIndex();
#endif
}

void VRGeometry::setMesh(OSGGeometryPtr g) {
    if (!g) {
        g = OSGGeometry::create( Geometry::create() );
        OSG::setName(g->geo, getName()+"_newMesh");
    }
    Reference ref;
    ref.type = CODE;
    setMesh(g, ref);
}

void VRGeometry::meshChanged() { lastMeshChange = VRGlobals::CURRENT_FRAME; }

void VRGeometry::setPrimitive(string parameters) {
    stringstream ss(parameters);
    string prim, args;
    ss >> prim;
    getline(ss, args);

    this->primitive = VRPrimitive::create(prim);
    if (this->primitive == 0) return;
    if (args != "") this->primitive->fromString(args);
    source.type = PRIMITIVE;
    source.parameter = prim + " " + this->primitive->toString();
    setMesh( OSGGeometry::create( this->primitive->make() ), source);
#if WASM
    getMaterial()->updateOGL2Shader();
#endif
}

/** Create a mesh using vectors with positions, normals, indices && optionaly texture coordinates **/
void VRGeometry::create(int type, vector<Vec3d> pos, vector<Vec3d> norms, vector<int> inds, vector<Vec2d> texs) {
    bool doTex = (texs.size() == pos.size());

    GeoUInt8PropertyMTRecPtr      Type = GeoUInt8Property::create();
    OSG::setName(Type, "VRGeometry_simple_setup_geo_type");
    GeoUInt32PropertyMTRecPtr     Length = GeoUInt32Property::create();
    GeoPnt3fPropertyMTRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyMTRecPtr      Norms = GeoVec3fProperty::create();
    GeoUInt32PropertyMTRecPtr     Indices = GeoUInt32Property::create();
    SimpleMaterialMTRecPtr        Mat = SimpleMaterial::create();
    GeoVec2fPropertyMTRecPtr      Tex = 0;
    if (doTex) Tex = GeoVec2fProperty::create();


    Type->addValue(type);
    Length->addValue(inds.size());

    //positionen und Normalen
    for (unsigned int i=0;i<pos.size();i++) {
            Pos->addValue(pos[i]);
            Norms->addValue(norms[i]);
            if (doTex) Tex->addValue(texs[i]);
    }

    for (unsigned int i=0;i<inds.size();i++) {
            Indices->addValue(inds[i]);
    }

    Mat->setDiffuse(Color3f(0.8,0.8,0.6));
    Mat->setAmbient(Color3f(0.4, 0.4, 0.2));
    Mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    GeometryMTRecPtr geo = Geometry::create();
    OSG::setName(geo, "simpleMesh");
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
    //if (Pos->size() == 1) Pos->addValue(Pnt3d()); // hack to avoid the single point bug
    mesh->geo->setPositions(Pos);
    mesh->geo->addAttachment(Pos, 0);
    meshChanged();
}

void VRGeometry::setColor(string c) {
    auto m = VRMaterial::get(c); // use get instead of create because of memory leak?
    m->setDiffuse(c);
    setMaterial(m);
}

void VRGeometry::setType(int t) {
    if (!meshSet) setMesh();
    GeoUInt8PropertyMTRecPtr Type = GeoUInt8Property::create();
    OSG::setName(Type, "VRGeometry_setType_type");
    Type->addValue(t);
    setTypes(Type);
}

void VRGeometry::makeUnique() {
    if (mesh_node == 0) return;
    NodeMTRecPtr clone = deepCloneTree( mesh_node->node );
    setMesh( OSGGeometry::create( dynamic_cast<Geometry*>( clone->getCore() ) ), source );
}

void VRGeometry::makeSingleIndex() {
    if (!mesh || !mesh->geo) return;
    if (!mesh->geo->isSingleIndex()) {

        VRGeoData data(ptr());
        data.makeSingleIndex();
    }
}

// OSG 2.0 function not implemented :(
// this is a port of the function in OSG 1.8
void calcFaceNormals(GeometryMTRecPtr geo) {
    if (!geo->getPositions() || geo->getPositions()->size() == 0) {
        cout << "Warning: no positions for calcFaceNormals\n";
        return;
    }

    GeoUInt32PropertyMTRecPtr newIndex = GeoUInt32Property::create();
    GeoVec3fPropertyMTRecPtr newNormals = GeoVec3fProperty::create();
    Vec3f normal;

    FaceIterator faceIter = geo->beginFaces();
    GeoIntegralPropertyMTRecPtr oldPosIndex = geo->getIndex(Geometry::PositionsIndex);
    GeoIntegralPropertyMTRecPtr oldNormsIndex = geo->getIndex(Geometry::NormalsIndex);

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

void VRGeometry::flipNormals() {
    if (!mesh || !mesh->geo) return;
    GeoVectorPropertyMTRecPtr normals = mesh->geo->getNormals();
    for (unsigned int i=0; i<normals->size(); i++) {
        Vec3f n = normals->getValue<Vec3f>(i);
        normals->setValue(-n, i);
    }
    //VRGeoData data(ptr()); // fails with for example sphere primitive
    //for (int i=0; i<data.getDataSize(4); i++) data.setNorm( i, - data.getNormal(i) );
}

int VRGeometry::getLastMeshChange() { return lastMeshChange; }

void VRGeometry::setTypes(GeoIntegralProperty* types) { if (!meshSet) setMesh(); mesh->geo->setTypes(types); mesh->geo->addAttachment(types, 1); }
void VRGeometry::setNormals(GeoVectorProperty* Norms) { if (!meshSet) setMesh(); mesh->geo->setNormals(Norms); mesh->geo->addAttachment(Norms, 2); }

void VRGeometry::fixColorMapping() {
    mesh->geo->setIndex(mesh->geo->getIndex(Geometry::PositionsIndex), Geometry::ColorsIndex);
}

void VRGeometry::setColors(GeoVectorProperty* Colors, bool fixMapping) {
    if (!meshSet) setMesh();
    mesh->geo->setColors(Colors);
    mesh->geo->addAttachment(Colors, 3);
    if (Colors && mesh->geo->getPositions()) {
        auto N1 = mesh->geo->getPositions()->size();
        auto N2 = Colors->size();
        if (N1 != N2) mesh->geo->setColors(0);
    }
    if (!Colors || Colors->size() == 0) fixColorMapping();
    if (fixMapping) fixColorMapping();
}

void VRGeometry::remColors(bool copyGeometry) {
    if (!meshSet) return;
    if (!mesh) return;
    if (!mesh->geo) return;
    if (!mesh->geo->getColors()) return;


    /*auto checkGeo = [&](Geometry* geo) {
        cout << "checkGeo " << geo << endl;
        cout << " t " << geo->getTypes          () << endl;
        cout << " l " << geo->getLengths        () << endl;
        cout << " p " << geo->getPositions      () << endl;
        cout << " n " << geo->getNormals        () << endl;
        cout << " c " << geo->getColors         () << endl;
        cout << " c " << geo->getSecondaryColors() << endl;
        cout << " t " << geo->getTexCoords      () << endl;
        cout << " t " << geo->getTexCoords1     () << endl;
        cout << " t " << geo->getTexCoords2     () << endl;
        cout << " t " << geo->getTexCoords3     () << endl;
        cout << " t " << geo->getTexCoords4     () << endl;
        cout << " t " << geo->getTexCoords5     () << endl;
        cout << " t " << geo->getTexCoords6     () << endl;
        cout << " t " << geo->getTexCoords7     () << endl;
        cout << " m " << geo->getMaterial     () << endl;
        cout << " i " << geo->isSingleIndex     () << endl;
        cout << " i  " << geo->getIndices     () << endl;
        cout << " ip " << geo->getIndex     (Geometry::PositionsIndex) << endl;
        cout << " in " << geo->getIndex     (Geometry::NormalsIndex) << endl;
        cout << " ic " << geo->getIndex     (Geometry::ColorsIndex) << endl;
        cout << " ic " << geo->getIndex     (Geometry::SecondaryColorsIndex) << endl;
        cout << " it " << geo->getIndex     (Geometry::TexCoordsIndex) << endl;
        cout << " it " << geo->getIndex     (Geometry::TexCoords1Index) << endl;
        cout << " it " << geo->getIndex     (Geometry::TexCoords2Index) << endl;
        cout << " it " << geo->getIndex     (Geometry::TexCoords3Index) << endl;
        cout << " it " << geo->getIndex     (Geometry::TexCoords4Index) << endl;
        cout << " it " << geo->getIndex     (Geometry::TexCoords5Index) << endl;
        cout << " it " << geo->getIndex     (Geometry::TexCoords6Index) << endl;
        cout << " it " << geo->getIndex     (Geometry::TexCoords7Index) << endl;
        cout << " ig " << geo->getClassicGLId     () << endl;
        cout << " ia " << geo->getAttGLId     () << endl;
        //cout << " iu " << geo->getUniqueIndexBag     () << endl;
        cout << " if " << geo->getFuncIdDrawElementsInstanced     () << endl;
        cout << " if " << geo->getFuncIdDrawArraysInstanced     () << endl;
        //cout << " ip " << geo->getPumpGroupStorage     () << endl;
    };*/

    //checkGeo(mesh->geo);
    //SceneFileHandler::the()->write(mesh_node->node, "temp1.osg");

    if (copyGeometry) {
        VRGeoData data(ptr());
        setMesh();
        data.apply(ptr());
    }

    mesh->geo->setColors(0);

    //checkGeo(mesh->geo);
}

void VRGeometry::setLengths(GeoIntegralProperty* lengths) { if (!meshSet) setMesh(); mesh->geo->setLengths(lengths); mesh->geo->addAttachment(lengths, 4); }
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
    mesh->geo->addAttachment(Tex, 5*10+i);
}

void VRGeometry::setPositionalTexCoords(float scale, int i, Vec3i format) {
    if (!mesh || ! mesh->geo) return;
    GeoVectorPropertyRefPtr pos = mesh->geo->getPositions();
    GeoVec3fPropertyRefPtr tex = GeoVec3fProperty::create();
    if (!pos) return;
    for (unsigned int i=0; i<pos->size(); i++) {
        auto p = Vec3d(pos->getValue<Pnt3f>(i))*scale;
        tex->addValue(Vec3d(p[format[0]], p[format[1]], p[format[2]]));
    }
    setTexCoords(tex, i, 1);
}

void VRGeometry::setPositionalTexCoords2D(float scale, int i, Vec2i format) {
    if (!mesh || !mesh->geo) return;
    GeoVectorPropertyRefPtr pos = mesh->geo->getPositions();
    if (!pos) return;
    GeoVec2fPropertyRefPtr tex = GeoVec2fProperty::create();
    for (unsigned int i=0; i<pos->size(); i++) {
        auto p = Vec3d(pos->getValue<Pnt3f>(i))*scale;
        tex->addValue(Vec2d(p[format[0]], p[format[1]]));
    }
    setTexCoords(tex, i, 1);
}

void VRGeometry::setIndices(GeoIntegralProperty* Indices, bool doLengths) {
    if (!meshSet) setMesh();
    if (!Indices) { setMesh(0); return; }
    if (Indices->size() == 0) setMesh(0);
    if (!mesh->geo->getLengths() || doLengths) {
        GeoUInt32PropertyMTRecPtr Length = GeoUInt32Property::create();
        Length->addValue(Indices->size());
        setLengths(Length);
    }
    mesh->geo->setIndices(Indices);
    mesh->geo->addAttachment(Indices, 6);
}

int VRGeometry::size() {
    if (!mesh || !mesh->geo) return 0;
    auto p = mesh->geo->getPositions();
    if (!p) return 0;
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

Vec3d morphColor3(const Vec3d& c) { return c; }
Vec3d morphColor3(const Vec4d& c) { return Vec3d(c[0], c[1], c[2]); }
Vec4d morphColor4(const Vec3d& c) { return Vec4d(c[0], c[1], c[2], 1); }
Vec4d morphColor4(const Vec4d& c) { return c; }

vector<VRGeometryPtr> VRGeometry::splitByVertexColors() {
    VRGeoData self(ptr());
    auto geos = self.splitByVertexColors(getMatrix());
    for (auto geo : geos) getParent()->addChild(geo);
    destroy();
    return geos;
}

void VRGeometry::merge(VRGeometryPtr geo, PosePtr pose) {
    if (!geo) return;
    if (!geo->mesh->geo) return;
    if (!meshSet) setMesh();

    Matrix4d M;
    if (pose) M = pose->asMatrix();
    else if (shareAncestry(geo)) M = getMatrixTo(geo);

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
    for (unsigned int k=0, i=0; i < (unsigned int)(self.size()); i++) {
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

void VRGeometry::genTexCoords(string mapping, float scale, int channel, shared_ptr<Pose> uvp) {
    GeoVec2fPropertyMTRecPtr tex = GeoVec2fProperty::create();
    Matrix4d uvp_inv;
    if (uvp) uvp_inv = uvp->asMatrix();
    if (uvp) uvp_inv.invert();

    auto pos = mesh->geo->getPositions();
    auto norms = mesh->geo->getNormals();
    int N = pos->size();

    Vec3d p;
    Vec3d n;
    Vec2d uv;

    auto cubeMapping = [&](Vec3d& p, Vec3d& n, Vec2d& uv) {
        if (abs(n[0]) > abs(n[1]) && abs(n[0]) > abs(n[2])) uv = Vec2d(p[1], p[2]);
        if (abs(n[1]) > abs(n[0]) && abs(n[1]) > abs(n[2])) uv = Vec2d(p[0], p[2]);
        if (abs(n[2]) > abs(n[0]) && abs(n[2]) > abs(n[1])) uv = Vec2d(p[0], p[1]);
    };

    auto sphereMapping = [&](Vec3d& p, Vec3d& n, Vec2d& uv) {
        float r = p.length();
        float theta = acos(p[1]/r);
        float phi   = atan(p[2]/p[0]);
        uv = Vec2d(abs(phi),abs(theta));
    };

    for (int i=0; i<N; i++) {
        Vec3d n = Vec3d(norms->getValue<Vec3f>(i));
        Vec3d p = Vec3d(pos->getValue<Pnt3f>(i));
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
    map<int, Pnt3d> collapse_points;
    map<int, Vec3d> collapse_normals;
	TriangleIterator it(mesh);
	for(int i=0; !it.isAtEnd(); ++it, i++) {
        float r = (float)rand()/RAND_MAX;
        if (r <= f) continue;
        Vec3i in(it.getPositionIndex(0), it.getPositionIndex(1), it.getPositionIndex(2));

        if (collapsing.count(in[0])) continue;
        if (collapsing.count(in[1])) continue;

        collapsing[in[0]] = in[1];
        collapsing[in[1]] = in[0];

        //Pnt3d p = (it.getPosition(0) + Vec3d(it.getPosition(1)))*0.5; // edge midpoint
        //Vec3d n = (it.getNormal(0) + it.getNormal(1))*0.5;

        Pnt3d p = it.getPosition(0);
        Vec3d n = it.getNormal(0);

        collapse_points[in[0]] = p;
        collapse_points[in[1]] = p;
        collapse_normals[in[0]] = n;
        collapse_normals[in[1]] = n;
	}

	GeoPnt3fPropertyMTRecPtr positions = GeoPnt3fProperty::create();
	GeoUInt32PropertyMTRecPtr indices = GeoUInt32Property::create();
	GeoVec3fPropertyMTRecPtr normals = GeoVec3fProperty::create();

	GeoUInt32PropertyMTRecPtr idx = dynamic_cast<GeoUInt32Property*>(mesh->getIndices());
	for (unsigned int i=0; i<idx->size(); i++) {
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

	GeoUInt32PropertyMTRecPtr lengths = GeoUInt32Property::create();
	lengths->addValue(indices->size());
	setLengths(lengths);


    createSharedIndex(mesh);*/
}

void VRGeometry::removeDoubles(float minAngle) {// TODO: use angle
    createSharedIndex(mesh->geo);
}

void VRGeometry::setRandomColors() {
    if (!mesh->geo) return;
    GeoPnt3fPropertyMTRecPtr pos = dynamic_cast<GeoPnt3fProperty*>(mesh->geo->getPositions());
    if (!pos) return;
	int N = pos->size();

	GeoVec4fPropertyMTRecPtr cols = GeoVec4fProperty::create();
	for (int i=0; i<N; i++) {
        Color4f c; c.setRandom();
        cols->addValue( c );
	}
    setColors(cols);
}

/** Returns the geometric center of the mesh **/
Vec3d VRGeometry::getGeometricCenter() {
    if (!meshSet) return Vec3d(0,0,0);
    GeoPnt3fPropertyMTRecPtr pos = dynamic_cast<GeoPnt3fProperty*>(mesh->geo->getPositions());

    Vec3d center = Vec3d(0,0,0);
    for (unsigned int i=0;i<pos->size();i++)
        center += Vec3d(pos->getValue(i));

    center *= 1./pos->size();

    return center;
}

/** Returns the average of all normals of the mesh (not normalized, can be zero) **/
Vec3d VRGeometry::getAverageNormal() {
    if (!meshSet) return Vec3d(0,1,0);
    GeoVec3fPropertyMTRecPtr norms = dynamic_cast<GeoVec3fProperty*>(mesh->geo->getNormals());

    Vec3d normal = Vec3d(0,0,0);
    for (unsigned int i=0;i<norms->size();i++) {
        normal += Vec3d(norms->getValue(i));
    }

    normal *= 1./norms->size();

    return normal;
}

void VRGeometry::influence(vector<Vec3d> pnts, vector<Vec3d> values, int power, float color_code, float dl_max) {
    interpolator inp;
    inp.setPoints(pnts);
    inp.setValues(values);
    if (color_code > 0) {
        if (mesh->geo->getColors() == 0) {
            GeoVec4fPropertyMTRecPtr cols = GeoVec4fProperty::create();
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
    GeoPnt3fPropertyMTRecPtr pos = dynamic_cast<GeoPnt3fProperty*>(mesh->geo->getPositions());

    float max = pos->getValue(0)[axis];
    for (unsigned int i=0;i<pos->size();i++) {
        if (max < pos->getValue(i)[axis]) max = pos->getValue(i)[axis];
    }

    return max;
}

/** Returns the minimum position on the x, y || z axis **/
float VRGeometry::getMin(int axis) {
    if (!meshSet) return 0;
    if (axis != 0 && axis != 1 && axis != 2) return 0;
    GeoPnt3fPropertyMTRecPtr pos = dynamic_cast<GeoPnt3fProperty*>(mesh->geo->getPositions());

    float min = pos->getValue(0)[axis];
    for (unsigned int i=0;i<pos->size();i++) {
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

    if (auto m = mesh->geo->getMaterial()) mesh->geo->subAttachment(m);
    mesh->geo->setMaterial(mat->getMaterial()->mat);
    mesh->geo->addAttachment(mat->getMaterial()->mat, 9);
#ifdef WASM
    mat->updateOGL2Shader();
#endif
}

VRMaterialPtr VRGeometry::getMaterial() { return mat; }

float VRGeometry::calcSurfaceArea() {
    if (!meshSet) return 0;

    float A = 0;
    TriangleIterator it(mesh->geo);

	for(int i=0; !it.isAtEnd(); ++it, i++) {
        auto p0 = it.getPosition(0);
        auto p1 = it.getPosition(1);
        auto p2 = it.getPosition(2);
        auto d1 = p1-p0;
        auto d2 = p2-p0;
        A += d1.cross(d2).length();
	}

    return 0.5*A;
}

void VRGeometry::setReference(Reference ref) { source = ref; }
VRGeometry::Reference VRGeometry::getReference() { return source; }

void VRGeometry::showGeometricData(string type, bool b) {
    if (dataLayer.count(type)) dataLayer[type]->destroy();

    VRGeometryPtr geo = VRGeometry::create("DATALAYER_"+getName()+"_"+type, true);
    dataLayer[type] = geo;
    addChild(geo);

    GeoColor3fPropertyMTRecPtr cols = GeoColor3fProperty::create();
    GeoPnt3fPropertyMTRecPtr pos = GeoPnt3fProperty::create();
    GeoUInt32PropertyMTRecPtr inds = GeoUInt32Property::create();

    Pnt3d p;
    Vec3d n;

    if (type == "Normals") {
        GeoVectorPropertyMTRecPtr g_norms = mesh->geo->getNormals();
        GeoVectorPropertyMTRecPtr g_pos = mesh->geo->getPositions();
        for (unsigned int i=0; i<g_norms->size(); i++) {
            p = Pnt3d(g_pos->getValue<Pnt3f>(i));
            n = Vec3d(g_norms->getValue<Vec3f>(i));
            pos->addValue(p);
            pos->addValue(p+n*0.1);
            cols->addValue(Vec3d(1,1,1));
            cols->addValue(Vec3d(abs(n[0]),abs(n[1]),abs(n[2])));
            inds->addValue(2*i);
            inds->addValue(2*i+1);
        }

        geo->setPositions(pos);
        geo->setType(GL_LINES);
        geo->setColors(cols);
        geo->setIndices(inds);
    }

    VRMaterialPtr m = VRMaterial::create("some-mat");
    geo->setMaterial(m);
    m->setLit(false);
}

void VRGeometry::setup(VRStorageContextPtr context) {
    string p1, p2, p3, p4;
    stringstream ss;
    VRGeometryPtr g;
    // get source info
    // construct data from that

    string bname = getBaseName();
    setName( "TMP_GEO_SETUP_NAME" );

    switch(source.type) {
        case CODE:
            break;
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
            setPrimitive(source.parameter);
            break;
    }

    setName( bname );
}

void VRGeometry::readSharedMemory(string segment, string object) {
#ifndef WITHOUT_SHARED_MEMORY
    VRSharedMemory sm(segment, false);

    int sm_state = sm.getObject<int>(object+"_state");
    while (sm.getObject<int>(object+"_state") == sm_state) {
        cout << "VRGeometry::readSharedMemory: waiting for data: " << sm_state << endl;
        std::this_thread::sleep_for(chrono::milliseconds(1));
    }

    // read buffer
    auto sm_types = sm.getVector<int>(object+"_types");
    auto sm_lengths = sm.getVector<int>(object+"_lengths");
    auto sm_pos = sm.getVector<float>(object+"_pos");
    auto sm_norms = sm.getVector<float>(object+"_norms");
    auto sm_inds = sm.getVector<int>(object+"_inds");
    auto sm_cols = sm.getVector<float>(object+"_cols");

    GeoPnt3fPropertyMTRecPtr pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyMTRecPtr norms = GeoVec3fProperty::create();
    GeoUInt32PropertyMTRecPtr inds = GeoUInt32Property::create();
    GeoUInt32PropertyMTRecPtr types = GeoUInt32Property::create();
    GeoUInt32PropertyMTRecPtr lengths = GeoUInt32Property::create();
    GeoVec4fPropertyMTRecPtr cols = GeoVec4fProperty::create();

    cout << "SM mesh read: " << sm_types.size() << " " << sm_lengths.size() << " " << sm_pos.size() << " " << sm_norms.size() << " " << sm_inds.size() << " " << sm_cols.size() << endl;

    if (sm_types.size() > 0) for (auto& t : sm_types) types->addValue(t);
    if (sm_lengths.size() > 0) for (auto& l : sm_lengths) lengths->addValue(l);
    for (auto& i : sm_inds) inds->addValue(i);
    if (sm_pos.size() > 0) for (unsigned int i=0; i<sm_pos.size()-2; i+=3) pos->addValue(Pnt3d(sm_pos[i], sm_pos[i+1], sm_pos[i+2]));
    if (sm_norms.size() > 0) for (unsigned int i=0; i<sm_norms.size()-2; i+=3) norms->addValue(Vec3d(sm_norms[i], sm_norms[i+1], sm_norms[i+2]));
    if (sm_cols.size() > 0) for (unsigned int i=0; i<sm_cols.size()-2; i+=3) cols->addValue(Pnt3d(sm_cols[i], sm_cols[i+1], sm_cols[i+2]));

    cout << "osg mesh data: " << types->size() << " " << lengths->size() << " " << pos->size() << " " << norms->size() << " " << inds->size() << " " << cols->size() << endl;

    unsigned int N = pos->size();
    if (N == 0) return;

    setTypes(types);
    setLengths(lengths);
    setPositions(pos);
    if (norms->size() == N) setNormals(norms);
    if (cols->size() == N) setColors(cols);
    setIndices(inds);
#endif
}


void VRGeometry::clear() {
    VRGeoData geo(ptr());
    geo.reset();
}

void VRGeometry::addPoint(int i) {
    VRGeoData geo(ptr());
    bool toApply = (geo.getNIndices() == 0);
    geo.pushPoint(i);
    if (toApply) geo.apply(ptr(), false);
}

void VRGeometry::addLine(Vec2i ij) {
    VRGeoData geo(ptr());
    bool toApply = (geo.getNIndices() == 0);
    geo.pushLine(ij[0], ij[1]);
    if (toApply) geo.apply(ptr(), false);
}

void VRGeometry::addTriangle(Vec3i ijk) {
    VRGeoData geo(ptr());
    bool toApply = (geo.getNIndices() == 0);
    geo.pushTri(ijk[0], ijk[1], ijk[2]);
    if (toApply) geo.apply(ptr(), false);
}

void VRGeometry::addQuad(Vec4i ijkl) {
    VRGeoData geo(ptr());
    bool toApply = (geo.getNIndices() == 0);
    geo.pushQuad(ijkl[0], ijkl[1], ijkl[2], ijkl[3]);
    if (toApply) geo.apply(ptr(), false);
}

void VRGeometry::convertToTrianglePatches() {
    if (!meshSet) return;

    TriangleIterator it(mesh->geo);
    VRGeoData data;

	for (int i=0; !it.isAtEnd(); ++it, i++) {
        data.pushVert(Pnt3d(it.getPosition(0)), Vec3d(it.getNormal(0)), Vec2d(it.getTexCoords(0,0)));
        data.pushVert(Pnt3d(it.getPosition(1)), Vec3d(it.getNormal(1)), Vec2d(it.getTexCoords(0,1)));
        data.pushVert(Pnt3d(it.getPosition(2)), Vec3d(it.getNormal(2)), Vec2d(it.getTexCoords(0,2)));
        data.pushTri();
	}

	data.apply(ptr());
	setType(GL_PATCHES);
	setPatchVertices(3);
}

void VRGeometry::convertToTriangles() {
    if (!meshSet) return;

    TriangleIterator it(mesh->geo);
    VRGeoData data;

    bool hasColor = (mesh->geo->getColors() != 0);
    bool hasTexCoords = (mesh->geo->getTexCoords() != 0);

	for (int i=0; !it.isAtEnd(); ++it, i++) {
        data.pushVert(Pnt3d(it.getPosition(0)), Vec3d(it.getNormal(0)));
        data.pushVert(Pnt3d(it.getPosition(1)), Vec3d(it.getNormal(1)));
        data.pushVert(Pnt3d(it.getPosition(2)), Vec3d(it.getNormal(2)));

        if (hasColor) {
            data.pushColor(it.getColor(0));
            data.pushColor(it.getColor(1));
            data.pushColor(it.getColor(2));
        }

        if (hasTexCoords) {
            data.pushTexCoord(Vec2d(it.getTexCoords(0)));
            data.pushTexCoord(Vec2d(it.getTexCoords(1)));
            data.pushTexCoord(Vec2d(it.getTexCoords(2)));
        }
        data.pushTri();
	}

	data.apply(ptr());
}



//todo flag ifEdge
// if not edge don't add first/last point to data
// if edge: only add every other point to data, but add all points to pntsOnEdge
vector<Pnt3d> VRGeometry::addPointsOnEdge(VRGeoData& data, int resolution, Pnt3d p1, Pnt3d p2, bool isEdge/*, float jitter*/) {
    vector<Pnt3d> pntsOnEdge;
    auto length = p1.dist(p2);

    Vec3d connection = Vec3d(p2 - p1);
    connection.normalize();
    int steps = int(length * resolution);
    if (steps < 1) steps = 1;
    auto stepSize = length/steps;

    for (int i = 0; i < steps; i++) {
        float t = i;
        if (i > 0) t += -0.5 + 1.0*rand()/float(RAND_MAX);
        Pnt3d p = p1 + connection * stepSize * t;
        pntsOnEdge.push_back(p);
        if (isEdge && (i%2 != 0) && (steps > 2)) continue;
        if (!isEdge && i == 0) continue;
        data.pushVert(p);
        data.pushPoint();
    }
    pntsOnEdge.push_back(p2);
    return pntsOnEdge;
}


vector< tuple<Pnt3d, Pnt3d>> VRGeometry::mapPoints(vector<Pnt3d>& e1, vector<Pnt3d>& e2) {
    vector< tuple<Pnt3d, Pnt3d>> mappedPoints;
    if (e1.size() < 3 || e2.size() < 3) return mappedPoints;
    //addPointsOnEdge for each match
    float stepSize1 = 100.0/(e1.size() - 1);
    float stepSize2 = 100.0/(e2.size() - 1);
    for (unsigned int i = 1; i< e1.size(); i++) {
        int k = round((i * stepSize1)/stepSize2) + 1;
        try {
            mappedPoints.push_back(make_tuple(e1[i], e2.at(e2.size()-k)));
        } catch (exception& e) {
            cout << "exception in mapPoints(): " << e.what() << endl;
        }
    }

    return mappedPoints;
}


VRPointCloudPtr VRGeometry::convertToPointCloud(map<string, string> options) {
    VRGeoData data;
    int resolution = 1;
    int partitionSize = -1;
    if (options.count("resolution")) resolution = toInt(options["resolution"]);
    if (options.count("partitionSize")) partitionSize = toInt(options["partitionSize"]);
    auto pointcloud = VRPointCloud::create("pointcloud");
    pointcloud->applySettings(options);

    if (!meshSet) return pointcloud;
    convertToTriangles();

    TriangleIterator it(mesh->geo);

    Color3f color;

	for (int i=0; !it.isAtEnd(); ++it, i++) {
        vector<Edge> edges = {};
        for (int j = 0; j < 3; j++) {
            Edge e{addPointsOnEdge(data, resolution, Pnt3d(it.getPosition(j)), Pnt3d(it.getPosition((j+1)%3)), true)};
            e.length = e.pnts.front().dist(e.pnts.back());
            edges.push_back(e);
        }
        sort(edges.begin(), edges.end(), [] (Edge left, Edge right) -> bool {return left.length < right.length;});
        if (edges[0].pnts.size() == 0) continue;

        //float area = edges[0].length * edges[1].length / 2;
        auto mappedPoints = mapPoints(edges[0].pnts, edges[2].pnts);
        for (auto& match : mappedPoints) addPointsOnEdge(data, resolution, get<0>(match), get<1>(match), false);
    }
    cout << "added " << data.size() << " new points!" << endl;

    Color3f col(1,0.5,0.5);
    for (int i = 0; i < data.size(); i++) {
        //pointcloud->getOctree()->add(Vec3d(data.getPosition(i)), new Color3f(0,0,0) );
        pointcloud->getOctree()->add(Vec3d(data.getPosition(i)), &color, -1, true, partitionSize );
    }
    pointcloud->setupLODs();

    return pointcloud;
}

OSG_END_NAMESPACE;
