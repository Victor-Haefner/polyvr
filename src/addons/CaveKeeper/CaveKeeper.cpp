#include "CaveKeeper.h"

#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGGeoProperties.h>

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/objects/VRCamera.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/setup/devices/VRDevice.h"

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

SimpleMaterialRecPtr BlockWorld::initMaterial(string texture) {
    if (materials->count(texture) == 1) return (*materials)[texture];

    //simple material
    SimpleMaterialRecPtr mat = SimpleMaterial::create();
    mat->setDiffuse(Color3f(0.8,0.5,0.1));

    //shader programs
    VRShader* wshader = new VRShader(mat);
    string wdir = VRSceneManager::get()->getOriginalWorkdir();
    wshader->setFragmentProgram(wdir+"/shader/Blockworld.fp");
    wshader->setVertexProgram(wdir+"/shader/Blockworld.vp");
    wshader->setGeometryProgram(wdir+"/shader/Blockworld.gp");

    //shader parameter
    float voxel = 1.0;
    wshader->addParameter("Vox0", Vec4f(voxel, 0.0, 0.0, 0.0) );
    wshader->addParameter("Vox1", Vec4f(0.0, voxel, 0.0, 0.0) );
    wshader->addParameter("Vox2", Vec4f(0.0, 0.0, voxel, 0.0) );

    wshader->addParameter("cam_pos", Vec4f(0.0, 0.0, 0.0, 0.0) );

    wshader->addParameter("texture", 0);
    wshader->addParameter("tc1", Vec2f(0,0));
    wshader->addParameter("tc2", Vec2f(1,0));
    wshader->addParameter("tc3", Vec2f(1,1));
    wshader->addParameter("tc4", Vec2f(0,1));

    //texture
    TextureEnvChunkRecPtr tex_env_chunk = TextureEnvChunk::create();

    TextureObjChunkRecPtr tex_obj_chunk = TextureObjChunk::create();
    VRTextureGenerator tgen;
    tgen.setSize(512,512);
    tgen.add(PERLIN, 1./2, Vec3f(0.3,0.1,0.1), Vec3f(0.9,0.5,0.1));
    tgen.add(PERLIN, 1./8, Vec3f(0.8,0.8,0.8), Vec3f(1.0,1.0,1.0));
    tgen.add(PERLIN, 1./32, Vec3f(0.8,0.8,0.8), Vec3f(1.0,1.0,1.0));
    ImageRecPtr img = tgen.compose(0);
    tex_obj_chunk->setImage(img);
    mat->addChunk(tex_obj_chunk);
    mat->addChunk(tex_env_chunk);

    tex_env_chunk->setEnvMode (GL_MODULATE);
    tex_obj_chunk->setMagFilter (GL_LINEAR);
    tex_obj_chunk->setMinFilter (GL_LINEAR_MIPMAP_LINEAR);
    tex_obj_chunk->setWrapS (GL_REPEAT);
    tex_obj_chunk->setWrapT (GL_REPEAT);

    (*shader)[texture] = wshader;
    (*materials)[texture] = mat;

    return mat;
}

VRGeometry* BlockWorld::createChunk(vector<octree2::element*>& elements) {

    GeometryRecPtr g = Geometry::create();
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

    for(uint i=0;i<elements.size();i++) {
		octree2::element* e = elements[i];
        for (int j=0;j<3;j++) {
            Pos->addValue(e->pos);

            Vec4f L1 = e->vertexLight[j];
            Vec4f L2 = e->vertexLight[j+3];

            Norms->addValue(Vec3f(L1[0], L1[1], L1[2]));
            Colors->addValue(Vec3f(L2[0], L2[1], L2[2]));
            texs->addValue(Vec3f(L1[3], L2[3]));
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

    VRGeometry* geo = new VRGeometry("chunk");
    geo->setMesh(g);
    return geo;
}

VRGeometry* BlockWorld::initChunk() {
	vector<octree2::element*>* elements = new vector<octree2::element*>();
	VRFunction<octree2::element*>* fkt = new VRFunction<octree2::element*>("blockworld_appendtovector", boost::bind(&BlockWorld::appendToVector, this, elements, _1));
    tree->traverse(fkt);
    delete fkt;

    VRGeometry* chunk = createChunk(*elements);
    delete elements;

    chunk->setMaterial(initMaterial("dirt"));

    return chunk;
}

void BlockWorld::appendToVector(vector<octree2::element*>* elements, octree2::element* e) {
    elements->push_back(e);
}

// update methods

void BlockWorld::updateShaderCamPos() {
    //VRTransform* e = VRSceneManager::get()->getTrackerUser(); // TODO
    VRTransform* e = 0;
    if (e == 0) e = VRSceneManager::getCurrent()->getActiveCamera();
    Vec4f cam_pos = Vec4f(e->getWorldPosition());

    map<string, VRShader*>::iterator itr = shader->begin();
    for (; itr != shader->end(); itr++)
        itr->second->setParameter("cam_pos", cam_pos);
}

BlockWorld::BlockWorld() {
    shader = new map<string, VRShader*>();
    materials = new map<string, SimpleMaterialRecPtr>();
    chunks = new map<int, VRGeometry*>();

    anchor = new VRObject("cavekeeper_anchor");
    anchor->addAttachment("dynamicaly_generated", 0);
    anchor->addAttachment("global", 0);
}

BlockWorld::~BlockWorld() {
    map<string, VRShader*>::iterator itr;
    for (itr = shader->begin(); itr != shader->end(); itr++) delete itr->second;

    delete shader;
    delete materials;
    delete chunks;

    delete anchor;
    delete tree;
}

void BlockWorld::initWorld() {
	tree = new octree2();
    createSphere(6, Vec3i(0,0,0));

    // TODO ?
    VRScene* scene = VRSceneManager::getCurrent();
    VRFunction<int>* ufkt = new VRFunction<int>("blockworld_update", boost::bind(&BlockWorld::updateShaderCamPos, this));
    scene->addUpdateFkt(ufkt, 1);

    (*chunks)[0] = initChunk();
    anchor->addChild(chunks->at(0));
}

VRObject* BlockWorld::getAnchor() { return anchor; }

void BlockWorld::redraw(int chunk_id) {
    VRGeometry* chunk = chunks->at(chunk_id);

    delete chunk;

    (*chunks)[chunk_id] = initChunk();
    anchor->addChild(chunks->at(chunk_id));
}


void CaveKeeper::placeLight(Vec3f p) {

    Vec3f normals[6];
    normals[0]= Vec3f(1,0,0);
    normals[1]= Vec3f(0,1,0);
    normals[2]= Vec3f(0,0,1);
    normals[3]= Vec3f(-1,0,0);
    normals[4]= Vec3f(0,-1,0);
    normals[5]= Vec3f(0,0,-1);

    Vec3f tangents[6];
    tangents[0] = Vec3f( 0, 1, 0);
    tangents[1] = Vec3f( 0, 0, 1);
    tangents[2] = Vec3f( 1, 0, 0);
    tangents[3] = Vec3f( 0, 1, 0);
    tangents[4] = Vec3f( 0, 0, 1);
    tangents[5] = Vec3f( 1, 0, 0);

    Vec2f ric[4];
    ric[0] = Vec2f(1,1);
    ric[1] = Vec2f(-1,1);
    ric[2] = Vec2f(1,-1);
    ric[3] = Vec2f(-1,-1);

	vector<octree2::element*> elements = tree->getAround(p, 10);
    for (uint i=0;i< elements.size();i++) {
		octree2::element* e = elements[i];

        Vec3f dp = p - e->pos;
        Vec3f n, p, t, x;
        for (int j=0;j<6;j++) {
            n = normals[j];
            t = tangents[j];
            x = t.cross(n);
            for (int k=0;k<4;k++) {
                p = dp + (n + ric[k][0]*t + ric[k][1]*x)*0.5;

                double a = 0.5 + 0.0*p.squareLength() + 1.0*p.length();
                p.normalize();

                float cos = -p*n;
                if (cos < 0) continue;

                float li = min(0.1+2.0*cos/a, 2.0);
                e->vertexLight[j][k] = max(li, e->vertexLight[j][k]);
            }
        }
    }
}

void CaveKeeper::dig(VRDevice* dev) {
    Line ray = dev->getBeacon()->castRay();
	octree2::element* e = tree->get(ray);
    if (e) {
        tree->addAround(e);
        tree->rem(e);
        redraw();
    }
}

void CaveKeeper::place(VRDevice* dev, string obj, VRTransform* geo) {
    if (dev == 0) return;

    Line ray = dev->getBeacon()->castRay();
	octree2::element* e = tree->get(ray);
    if (e) {
        Vec3f n = tree->getHitNormal();
        Vec3f p = e->pos + n;

        if (obj == "lantern") placeLight(p);

        geo = (VRTransform*)geo->duplicate(true);
        geo->setFrom(p);
        geo->setAt(e->pos);
        geo->addAttachment("dynamicaly_generated", 0);

        redraw();
    }
}

CaveKeeper::CaveKeeper() { initWorld(); }
CaveKeeper::~CaveKeeper() { ; }


OSG_END_NAMESPACE
