#include "VRGeometry.h"
#include <libxml++/nodes/element.h>

#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.
#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGGeoFunctions.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGTriangleIterator.h>
#include "core/scene/VRSceneLoader.h"
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
    geo->setPickable(isPickable());
    geo->setMatrix(getMatrix());
    return geo;
}

/** initialise a geometry object with his name **/
VRGeometry::VRGeometry(string name) : VRTransform(name) {
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
    /*this->primitive = VRPrimitive::make(primitive);
    if (this->primitive == 0) return;
    if (args != "") this->primitive->fromString(args);
    source.type = PRIMITIVE;
    source.parameter = primitive + " " + this->primitive->toString();
    setMesh(this->primitive->make(), source);*/
}

/** Create a mesh using vectors with positions, normals, indices and optionaly texture coordinates **/
void VRGeometry::create(int type, vector<Vec3f> pos, vector<Vec3f> norms, vector<int> inds, vector<Vec2f> texs) {
/*
    GeoUInt8PropertyRecPtr      Type = GeoUInt8Property::create();
    GeoUInt32PropertyRefPtr     Length = GeoUInt32Property::create();
    GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRefPtr      Norms = GeoVec3fProperty::create();
    GeoUInt32PropertyRefPtr     Indices = GeoUInt32Property::create();
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

    setMesh(geo);*/
}

/** Create a mesh using vectors with positions, normals, indices and optionaly texture coordinates **/
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

void VRGeometry::setTypes(GeoIntegralProperty* types) { if (!meshSet) setMesh(Geometry::create()); mesh->setTypes(types); }
void VRGeometry::setNormals(GeoVectorProperty* Norms) { if (!meshSet) setMesh(Geometry::create()); mesh->setNormals(Norms); }
void VRGeometry::setColors(GeoVectorProperty* Colors) { if (!meshSet) setMesh(Geometry::create()); mesh->setColors(Colors); }
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
    GeoUInt32PropertyRefPtr Length = GeoUInt32Property::create();
    Length->addValue(Indices->size());
    mesh->setLengths(Length);
    mesh->setIndices(Indices);
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

/** Returns the maximum position on the x, y or z axis **/
float VRGeometry::getMax(int axis) {
    if (!meshSet) return 0;
    if (axis != 0 and axis != 1 and axis != 2) return 0;
    GeoPnt3fPropertyRecPtr pos = dynamic_cast<GeoPnt3fProperty*>(mesh->getPositions());

    float max = pos->getValue(0)[axis];
    for (uint i=0;i<pos->size();i++) {
        if (max < pos->getValue(i)[axis]) max = pos->getValue(i)[axis];
    }

    return max;
}

/** Returns the minimum position on the x, y or z axis **/
float VRGeometry::getMin(int axis) {
    if (!meshSet) return 0;
    if (axis != 0 and axis != 1 and axis != 2) return 0;
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
    mesh->setMaterial(mat);
}

VRMaterial* VRGeometry::getMaterial() {
    if (!meshSet) return 0;
    return mat;
}

VRGeometry::Reference VRGeometry::getReference() { return source; }

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
    GeometryRecPtr g;
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
            g = VRSceneLoader::get()->loadGeometry(p1, p2);
            if (g == 0) {
                cout << "\n Could not find " << getName() << " in file " << p1 << endl;
                return;
            }
            setMesh( g, source, true );
            break;
        case PRIMITIVE:
            ss << source.parameter;
            ss >> p1; getline(ss, p2);
            setPrimitive(p1, p2);
            break;
    }
}

OSG_END_NAMESPACE;
