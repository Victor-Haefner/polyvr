#include "CaveKeeper.h"

#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGSimpleMaterial.h>

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/setup/devices/VRDevice.h"
#include "core/objects/VRCamera.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"



OSG_BEGIN_NAMESPACE;
using namespace std;

void BlockWorld::createPlane(int w) {
    for (int i=-w; i<=w; i++)
        for (int j=-w; j<=w;j++) tree->add(Vec3i(i,-2,j));
}

void BlockWorld::createSphere(int r, Vec3i p0) {
    for (int i=-r; i<=r; i++) {
        for (int j=-r; j<=r;j++) {
            for (int k=-r; k<=r;k++) {

                float e = i*i+j*j+k*k;
                Vec3i v = Vec3i(i,k,j);

                if (abs(e - r*r) < r) tree->add(v + p0);
            }
        }
    }

    for (int i=-r; i<=r; i++) {
        for (int j=-r; j<=r;j++) {
            for (int k=-r; k<=r;k++) {

                float e = i*i+j*j+k*k;
                Vec3i v = Vec3i(i,k,j);
                if (e < r*r) tree->setEmpty(v + p0);
            }
        }
    }
}

// mesh methods

VRMaterialPtr BlockWorld::initMaterial(string texture) {
    if (materials.count(texture) == 1) return materials[texture];

    string wdir = VRSceneManager::get()->getOriginalWorkdir();

    //simple material
    VRMaterialPtr mat = VRMaterial::create("cavekeeper_mat");
    mat->setDiffuse(Color3f(0.8,0.5,0.1));
    mat->readFragmentShader(wdir+"/shader/Blockworld.fp");
    mat->readVertexShader(wdir+"/shader/Blockworld.vp");
    mat->readGeometryShader(wdir+"/shader/Blockworld.gp");

    //shader parameter
    float voxel = 1.0;
    mat->setShaderParameter("Vox0", Vec4f(voxel, 0.0, 0.0, 0.0) );
    mat->setShaderParameter("Vox1", Vec4f(0.0, voxel, 0.0, 0.0) );
    mat->setShaderParameter("Vox2", Vec4f(0.0, 0.0, voxel, 0.0) );

    mat->setShaderParameter("cam_pos", Vec4f(0.0, 0.0, 0.0, 0.0) );

    mat->setShaderParameter("texture", 0);
    mat->setShaderParameter("tc1", Vec2f(0,0));
    mat->setShaderParameter("tc2", Vec2f(1,0));
    mat->setShaderParameter("tc3", Vec2f(1,1));
    mat->setShaderParameter("tc4", Vec2f(0,1));

    //texture
    VRTextureGenerator tgen;
    tgen.setSize(512,512);
    tgen.add(PERLIN, 1./2, Color4f(0.3,0.1,0.1,1), Color4f(0.9,0.5,0.1,1));
    tgen.add(PERLIN, 1./8, Color4f(0.8,0.8,0.8,1), Color4f(1.0,1.0,1.0,1));
    tgen.add(PERLIN, 1./32, Color4f(0.8,0.8,0.8,1), Color4f(1.0,1.0,1.0,1));
    mat->setTexture( tgen.compose(0) );
    mat->setTextureParams(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_MODULATE, GL_REPEAT, GL_REPEAT);

    materials[texture] = mat;
    return mat;
}

VRGeometryPtr BlockWorld::createChunk(vector<CKOctree::element*>& elements) {

    GeometryMTRecPtr g = Geometry::create();
    GeoUInt8PropertyRecPtr      Type = GeoUInt8Property::create();
    GeoUInt32PropertyRecPtr     Length = GeoUInt32Property::create();
    GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr      Norms = GeoVec3fProperty::create();
    GeoVec3fPropertyRecPtr      Colors = GeoVec3fProperty::create();
    GeoVec2fPropertyRefPtr      texs = GeoVec2fProperty::create();
    GeoUInt32PropertyRecPtr     Indices = GeoUInt32Property::create();
    SimpleMaterialRecPtr        Mat = SimpleMaterial::create();

    GeoVec4fPropertyRefPtr      vLights = GeoVec4fProperty::create();

    Type->addValue(GL_POINTS);
    Length->addValue(3*elements.size());

    int c = 0;

    for(unsigned int i=0;i<elements.size();i++) {
		CKOctree::element* e = elements[i];
        for (int j=0;j<3;j++) {
            Pos->addValue(e->pos);

            Vec4d L1 = e->vertexLight[j];
            Vec4d L2 = e->vertexLight[j+3];

            Norms->addValue(Vec3d(L1[0], L1[1], L1[2]));
            Colors->addValue(Vec3d(L2[0], L2[1], L2[2]));
            texs->addValue(Vec3d(L1[3], L2[3]));
            Indices->addValue(c);
            c++;
        }
    }


    g->setTypes(Type);
    g->setLengths(Length);
    g->setPositions(Pos);

    g->setNormals(Norms);
    g->setColors(Colors);
    g->setTexCoords(texs);
    g->setIndices(Indices);

    g->setMaterial(Mat);

    VRGeometryPtr geo = VRGeometry::create("chunk");
    geo->setMesh( OSGGeometry::create(g) );
    return geo;
}

VRGeometryPtr BlockWorld::initChunk() {
	vector<CKOctree::element*>* elements = new vector<CKOctree::element*>();
	VRFunction<CKOctree::element*>* fkt = new VRFunction<CKOctree::element*>("blockworld_appendtovector", bind(&BlockWorld::appendToVector, this, elements, _1));
    tree->traverse(fkt);
    delete fkt;

    VRGeometryPtr chunk = createChunk(*elements);
    delete elements;

    chunk->setMaterial(initMaterial("dirt"));

    return chunk;
}

void BlockWorld::appendToVector(vector<CKOctree::element*>* elements, CKOctree::element* e) {
    elements->push_back(e);
}

// update methods

void BlockWorld::updateShaderCamPos() {
    //VRTransformPtr e = VRSceneManager::get()->getTrackerUser(); // TODO
    VRTransformPtr e = 0;
    auto scene = VRScene::getCurrent();
    if (e == 0 && scene) e = scene->getActiveCamera();
    Vec4f cam_pos = Vec4f(e->getWorldPosition());
    for (auto m : materials) m.second->setShaderParameter("cam_pos", cam_pos);
}

BlockWorld::BlockWorld() {
    anchor = VRObject::create("cavekeeper_anchor");
    anchor->setPersistency(0);
}

BlockWorld::~BlockWorld() {}

void BlockWorld::initWorld() {
	tree = new CKOctree();
    createSphere(6, Vec3i(0,0,0));

    // TODO ?
    auto scene = VRScene::getCurrent();
    updatePtr = VRUpdateCb::create("blockworld_update", bind(&BlockWorld::updateShaderCamPos, this));
    if (scene) scene->addUpdateFkt(updatePtr, 1);

    chunks[0] = initChunk();
    anchor->addChild(chunks[0]);
}

VRObjectPtr BlockWorld::getAnchor() { return anchor; }

void BlockWorld::redraw(int chunk_id) {
    if (chunks.count(chunk_id)) chunks[chunk_id]->destroy();
    chunks[chunk_id] = initChunk();
    anchor->addChild(chunks[chunk_id]);
}

void CaveKeeper::placeLight(Vec3d p) {
    auto l = tree->addLight(p);
	auto elements = tree->getAround(p, 10);
	for (auto e : elements) e->updateLightning(l);
}

int CaveKeeper::intersect(VRDevicePtr dev) {
    Line ray = dev->getBeacon()->castRay();
	CKOctree::element* e = tree->get(ray);
	return e ? e->ID : 0;
}

int CaveKeeper::addBlock(Vec3i p) {
    return tree->add(p)->ID;
}

void CaveKeeper::remBlock(int i) {
    auto e = tree->getElement(i);
    if (e) {
        tree->addAround(e);
        tree->rem(e);
        redraw();
    }
}

void CaveKeeper::place(VRDevicePtr dev, string obj, VRTransformPtr geo) {
    if (dev == 0) return;

    Line ray = dev->getBeacon()->castRay();
	CKOctree::element* e = tree->get(ray);
    if (e) {
        Vec3d n = tree->getHitNormal();
        Vec3d p = e->pos + n;

        if (obj == "lantern") placeLight(p);

        geo = static_pointer_cast<VRTransform>( geo->duplicate(true) );
        geo->setVisible(true);
        geo->setFrom(p);
        geo->setAt(e->pos);
        geo->setPersistency(0);

        redraw();
    }
}

CaveKeeper::CaveKeeper() { initWorld(); }
CaveKeeper::~CaveKeeper() { ; }

void CaveKeeper::init(VRObjectPtr a) {
    a->addChild(getAnchor());
}

shared_ptr<CaveKeeper> CaveKeeper::create() { return shared_ptr<CaveKeeper>(new CaveKeeper()); }

OSG_END_NAMESPACE
