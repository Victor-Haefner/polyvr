#include "VRGeometry.h"
#include <libxml++/nodes/element.h>

#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.
#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGMultiPassMaterial.h>
#include <OpenSG/OSGGeoFunctions.h>
#include <OpenSG/OSGNameAttachment.h>

//#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGTypedGeoIntegralProperty.h>
#include <OpenSG/OSGTypedGeoVectorProperty.h>

#include <OpenSG/OSGTriangleIterator.h>
#include "core/scene/import/VRImport.h"
#include "core/math/interpolator.h"
#include "core/utils/toString.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/object/VRObjectT.h"
#include "VRPrimitive.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRObject* VRGeometry::copy(vector<VRObject*> children) {
    VRGeometry* geo = new VRGeometry(getBaseName());
    geo->setMesh(mesh);
    geo->setMaterial(mat);
    geo->source = source;
    geo->setVisible(isVisible());
    geo->setPickable(isPickable());
    geo->setMatrix(getMatrix());
    return geo;
}

/** initialise a geometry object with his name **/
VRGeometry::VRGeometry(string name) : VRTransform(name) {
    type = "Geometry";
    addAttachment("geometry", 0);
}

VRGeometry::VRGeometry(string name, bool hidden) : VRTransform(name) {
    setNameSpace("system");
    setIntern(hidden);
    type = "Geometry";
    addAttachment("geometry", 0);
}

VRGeometry::~VRGeometry() {
    ;
}

/** Set the geometry mesh (OSG geometry core) **/
void VRGeometry::setMesh(GeometryRecPtr g, Reference ref, bool keep_material) {
    if (g == 0) return;
    if (mesh_node) getNode()->subChild(mesh_node);

    mesh = g;
    mesh_node = makeNodeFor(g);
    OSG::setName(mesh_node, getName());
    getNode()->addChild(mesh_node);
    meshSet = true;
    source = ref;

    if (mat == 0) mat = VRMaterial::getDefault();
    if (keep_material) mat = VRMaterial::get(g->getMaterial());
    setMaterial(mat);
}

void VRGeometry::setMesh(GeometryRecPtr g) {
    Reference ref;
    ref.type = CODE;
    setMesh(g, ref);
}

void VRGeometry::setPrimitive(string primitive, string args) {
    this->primitive = VRPrimitive::make(primitive);
    if (this->primitive == 0) return;
    if (args != "") this->primitive->fromString(args);
    source.type = PRIMITIVE;
    source.parameter = primitive + " " + this->primitive->toString();
    setMesh(this->primitive->make(), source);
}

/** Create a mesh using vectors with positions, normals, indices && optionaly texture coordinates **/
void VRGeometry::create(int type, vector<Vec3f> pos, vector<Vec3f> norms, vector<int> inds, vector<Vec2f> texs) {
    GeoUInt8PropertyRecPtr      Type = GeoUInt8Property::create();
    GeoUInt32PropertyRecPtr     Length = GeoUInt32Property::create();
    GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr      Norms = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr     Indices = GeoUInt32Property::create();
    SimpleMaterialRecPtr        Mat = SimpleMaterial::create();
    GeoVec2fPropertyRecPtr      Tex = GeoVec2fProperty::create();

    Type->addValue(type);
    Length->addValue(inds.size());

    //positionen und Normalen
    for(uint i=0;i<pos.size();i++) {
            Pos->addValue(pos[i]);
            Norms->addValue(norms[i]);
            if (texs.size() == pos.size()) Tex->addValue(texs[i]);
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
    geo->setTexCoords(Tex);
    geo->setMaterial(Mat);

    setMesh(geo);
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
    if (!meshSet) setMesh(Geometry::create());
    if (Pos->size() == 1) Pos->addValue(Pnt3f()); // hack to avoid the single point bug
    mesh->setPositions(Pos);
}

void VRGeometry::setType(int t) {
    if (!meshSet) setMesh(Geometry::create());
    GeoUInt8PropertyRecPtr Type = GeoUInt8Property::create();
    Type->addValue(t);
    setTypes(Type);
}

void VRGeometry::makeUnique() {
    if (mesh_node == 0) return;
    NodeRecPtr clone = deepCloneTree( mesh_node );
    setMesh( dynamic_cast<Geometry*>( clone->getCore() ), source );
}

void VRGeometry::fixColorMapping() {
    mesh->setIndex(mesh->getIndex(Geometry::PositionsIndex), Geometry::ColorsIndex);
}

void VRGeometry::setTypes(GeoIntegralProperty* types) { if (!meshSet) setMesh(Geometry::create()); mesh->setTypes(types); }
void VRGeometry::setNormals(GeoVectorProperty* Norms) { if (!meshSet) setMesh(Geometry::create()); mesh->setNormals(Norms); }
void VRGeometry::setColors(GeoVectorProperty* Colors, bool fixMapping) { if (!meshSet) setMesh(Geometry::create()); mesh->setColors(Colors); if (fixMapping) fixColorMapping(); }
void VRGeometry::setLengths(GeoIntegralProperty* lengths) { if (!meshSet) setMesh(Geometry::create()); mesh->setLengths(lengths); }
void VRGeometry::setTexCoords(GeoVectorProperty* Tex, int i) {
    if (!meshSet) setMesh(Geometry::create());
    if (i == 0) mesh->setTexCoords(Tex);
    if (i == 1) mesh->setTexCoords1(Tex);
    if (i == 2) mesh->setTexCoords2(Tex);
    if (i == 3) mesh->setTexCoords3(Tex);
    if (i == 4) mesh->setTexCoords4(Tex);
    if (i == 5) mesh->setTexCoords5(Tex);
    if (i == 6) mesh->setTexCoords6(Tex);
    if (i == 7) mesh->setTexCoords7(Tex);
}

void VRGeometry::setIndices(GeoIntegralProperty* Indices) {
    if (!meshSet) setMesh(Geometry::create());
    if (Indices->size() == 0) setMesh(0);
    GeoUInt32PropertyRecPtr Length = GeoUInt32Property::create();
    Length->addValue(Indices->size());
    mesh->setLengths(Length);
    mesh->setIndices(Indices);
}

GeoVec4fPropertyRecPtr convertColors(GeoVectorProperty* v) {
    GeoVec4fPropertyRecPtr res = GeoVec4fProperty::create();
    if (v == 0) return res;
    if (v->size() == 0) return res;

    return res;

    int cN = sizeof(v[0]);
    if (cN == sizeof(Vec4f)) res = (GeoVec4fProperty*)v;
    if (cN == sizeof(Vec3f)) for (uint i=0; i<v->size(); i++) res->addValue( Vec4f(v->getValue<Vec3f>(i)) );
    return res;
}

void VRGeometry::merge(VRGeometry* geo) {
    if (!meshSet) {
        setIndices(GeoUInt32PropertyRecPtr( GeoUInt32Property::create()) );
        setTypes(GeoUInt8PropertyRecPtr( GeoUInt8Property::create()) );
        setNormals(GeoVec3fPropertyRecPtr( GeoVec3fProperty::create()) );
        if (geo->mesh->getColors()) setColors(GeoVec4fPropertyRecPtr( GeoVec4fProperty::create()) );
        setLengths(GeoUInt32PropertyRecPtr( GeoUInt32Property::create()) );
        setPositions(GeoPnt3fPropertyRecPtr( GeoPnt3fProperty::create()) );
    }

    Matrix M = getWorldMatrix();
    M.invert();
    M.mult( geo->getWorldMatrix() );

    GeoVectorProperty *v1, *v2;
    v1 = mesh->getPositions();
    v2 = geo->mesh->getPositions();
    int N = v1->size();
    for (uint i=0; i<v2->size(); i++) {
        Pnt3f p = v2->getValue<Pnt3f>(i);
        M.mult(p,p);
        v1->addValue(p);
    }

    v1 = mesh->getNormals();
    v2 = geo->mesh->getNormals();
    for (uint i=0; i<geo->mesh->getPositions()->size(); i++) v1->addValue(v2->getValue<Vec3f>(i));

    if (mesh->getColors()) {
        GeoVec4fPropertyRecPtr c1 = convertColors(mesh->getColors());
        GeoVec4fPropertyRecPtr c2 = convertColors(geo->mesh->getColors());
        for (uint i=0; i<c2->size(); i++) v1->addValue(c2->getValue(i));
    }

    GeoIntegralProperty *i1, *i2;
    i1 = mesh->getTypes();
    i2 = geo->mesh->getTypes();
    for (uint i=0; i<i2->size(); i++) i1->addValue(i2->getValue(i));

    i1 = mesh->getLengths();
    i2 = geo->mesh->getLengths();
    for (uint i=0; i<i2->size(); i++) i1->addValue(i2->getValue(i));

    i1 = mesh->getIndices();
    i2 = geo->mesh->getIndices();
    for (uint i=0; i<i2->size(); i++) i1->addValue(i2->getValue(i) + N);

    /*cout << "merge sizes:\n";
    if (mesh->getPositions()) cout << " pos: " << mesh->getPositions()->size() << endl;
    else cout << "no positions\n";
    if (mesh->getNormals()) cout << " norm: " << mesh->getNormals()->size() << endl;
    else cout << "no normals\n";
    if (mesh->getColors()) cout << " cols: " << mesh->getColors()->size() << endl;
    else cout << "no colors\n";
    if (mesh->getTypes()) cout << " types: " << mesh->getTypes()->size() << endl;
    else cout << "no types\n";
    if (mesh->getLengths()) {
        cout << " lengths: " << mesh->getLengths()->size() << endl;
        int lsum = 0;
        for (int i=0; i<mesh->getLengths()->size(); i++) lsum += mesh->getLengths()->getValue<int>(i);
        cout << " lengths sum: " << lsum << endl;
    } else cout << "no lengths\n";
    if (mesh->getIndices()) cout << " inds: " << mesh->getIndices()->size() << endl;
    else cout << "no indices\n";
    cout << "pos   idx " << mesh->getIndex(Geometry::PositionsIndex) << " " << geo->mesh->getIndex(Geometry::PositionsIndex) << endl;
    cout << "norms idx " << mesh->getIndex(Geometry::NormalsIndex) << " " << geo->mesh->getIndex(Geometry::NormalsIndex) << endl;
    cout << endl;*/
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
    createSharedIndex(mesh);
}

void VRGeometry::setRandomColors() {
    GeoPnt3fPropertyRecPtr pos = dynamic_cast<GeoPnt3fProperty*>(mesh->getPositions());
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
    GeoPnt3fPropertyRecPtr pos = dynamic_cast<GeoPnt3fProperty*>(mesh->getPositions());

    Vec3f center = Vec3f(0,0,0);
    for (uint i=0;i<pos->size();i++)
        center += Vec3f(pos->getValue(i));

    center *= 1./pos->size();

    return center;
}

/** Returns the average of all normals of the mesh (not normalized, can be zero) **/
Vec3f VRGeometry::getAverageNormal() {
    if (!meshSet) return Vec3f(0,1,0);
    GeoVec3fPropertyRecPtr norms = dynamic_cast<GeoVec3fProperty*>(mesh->getNormals());

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
        if (mesh->getColors() == 0) {
            GeoVec4fPropertyRecPtr cols = GeoVec4fProperty::create();
            cols->resize(mesh->getPositions()->size());
            setColors(cols);
            fixColorMapping();
        }
        inp.evalVec(mesh->getPositions(), power, mesh->getColors(), color_code, dl_max);
    }
    else inp.evalVec(mesh->getPositions(), power);
}

/** Returns the maximum position on the x, y || z axis **/
float VRGeometry::getMax(int axis) {
    if (!meshSet) return 0;
    if (axis != 0 && axis != 1 && axis != 2) return 0;
    GeoPnt3fPropertyRecPtr pos = dynamic_cast<GeoPnt3fProperty*>(mesh->getPositions());

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
    GeoPnt3fPropertyRecPtr pos = dynamic_cast<GeoPnt3fProperty*>(mesh->getPositions());

    float min = pos->getValue(0)[axis];
    for (uint i=0;i<pos->size();i++) {
        if (min > pos->getValue(i)[axis]) min = pos->getValue(i)[axis];
    }

    return min;
}

/** Returns the mesh as a OSG geometry core **/
GeometryRecPtr VRGeometry::getMesh() {
    if(meshSet) return mesh;
    else return 0;
}

VRPrimitive* VRGeometry::getPrimitive() { return primitive; }

void VRGeometry::setMeshVisibility(bool b) {
    if (!mesh_node) return;
    if (b) mesh_node->setTravMask(0xffffffff);
    else mesh_node->setTravMask(0);
}

/** Set the material of the mesh **/
void VRGeometry::setMaterial(VRMaterial* mat) {
    if (!meshSet) return;
    if (mat == 0) mat = this->mat;
    if (mat == 0) return;

    this->mat = mat;
    mesh->setMaterial(mat->getMaterial());
}

void VRGeometry::setMaterial(MaterialRecPtr mat) {
    if (!meshSet) return;
    if (mat == 0) return;

    if (this->mat == 0) this->mat = new VRMaterial("mat");
    this->mat->setMaterial(mat);

    setMaterial(this->mat);
}

VRMaterial* VRGeometry::getMaterial() {
    if (!meshSet) return 0;
    return mat;
}

VRGeometry::Reference VRGeometry::getReference() { return source; }

void VRGeometry::showGeometricData(string type, bool b) {
    if (dataLayer.count(type)) dataLayer[type]->destroy();

    VRGeometry* geo = new VRGeometry("DATALAYER_"+getName()+"_"+type, true);
    dataLayer[type] = geo;
    addChild(geo);

    GeoColor3fPropertyRecPtr cols = GeoColor3fProperty::create();
    GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
    GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();

    Pnt3f p;
    Vec3f n;

    if (type == "Normals") {
        GeoVectorPropertyRecPtr g_norms = mesh->getNormals();
        GeoVectorPropertyRecPtr g_pos = mesh->getPositions();
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

    VRMaterial* m = new VRMaterial("some-mat");
    geo->setMaterial(m);
    m->setLit(false);
}

void VRGeometry::saveContent(xmlpp::Element* e) {
    VRTransform::saveContent(e);
    stringstream ss; ss << source.type;
    e->set_attribute("sourcetype", ss.str());
    e->set_attribute("sourceparam", source.parameter);
}

void VRGeometry::loadContent(xmlpp::Element* e) {
    VRTransform::loadContent(e);

    source.type = toInt(e->get_attribute("sourcetype")->get_value().c_str());
    source.parameter = e->get_attribute("sourceparam")->get_value();

    string p1, p2;
    stringstream ss;
    VRGeometry* g;
    // get source info
    // construct data from that

    switch(source.type) {
        case CODE:
            return;
        case SCRIPT:
            break;
        case FILE:
            ss << source.parameter;
            ss >> p1; ss >> p2;
            g = VRImport::get()->loadGeometry(p1, p2);
            if (g) setMesh( g->getMesh(), source, true );
            else cout << "failed to load " << p2 << " from file " << p1 << endl;
            break;
        case PRIMITIVE:
            ss << source.parameter;
            ss >> p1; getline(ss, p2);
            setPrimitive(p1, p2);
            break;
    }
}

OSG_END_NAMESPACE;
