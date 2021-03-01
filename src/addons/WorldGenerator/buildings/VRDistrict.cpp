#include "VRDistrict.h"
#include "../VRWorldGenerator.h"

#include "VRBuilding.h"
#include "addons/WorldGenerator/GIS/OSMMap.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "core/objects/material/VRShader.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#ifndef WITHOUT_BULLET
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRSpatialCollisionManager.h"
#endif
#include "core/utils/toString.h"
#include "core/utils/system/VRSystem.h"
#include "core/math/triangulator.h"
#include "core/scene/VRSceneManager.h"

#define GLSL(shader) #shader

using namespace OSG;

VRDistrict::VRDistrict() : VRObject("District") {}
VRDistrict::~VRDistrict() {}

VRDistrictPtr VRDistrict::create() {
    auto d = VRDistrictPtr( new VRDistrict() );
    d->init();
    return d;
}

void VRDistrict::init() {
    if (!b_mat) {
        b_mat = VRMaterial::create("Buildings");
        string tex = "world/textures/Buildings.png";
        if (exists(tex)) b_mat->setTexture(tex, false);
        b_mat->setAmbient(Color3f(0.7, 0.7, 0.7)); //light reflection in all directions
        b_mat->setDiffuse(Color3f(1.0, 1.0, 1.0)); //light from ambient (without lightsource)
        b_mat->setSpecular(Color3f(0.2, 0.2, 0.2)); //light reflection in camera direction
        b_mat->setVertexShader(matVShdr, "buildingVS");
        b_mat->setFragmentShader(matFShdr, "buildingFS");
        b_mat->setFragmentShader(matFDShdr, "buildingFS", true);
        b_mat->setMagMinFilter(GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, 0);
        texture = VRTextureMosaic::create();
    }

    if (facades) facades->destroy();
    if (roofs) roofs->destroy();
    facades = VRGeometry::create("facades");
    roofs = VRGeometry::create("roofs");
    addChild(facades);
    addChild(roofs);
    facades->setMaterial(b_mat);
    roofs->setMaterial(b_mat);
}

VRDistrictPtr VRDistrict::ptr() { return dynamic_pointer_cast<VRDistrict>(shared_from_this()); }

Vec4d VRDistrict::getChunkUV(string type, int i) {
    if (!chunkIDs.count(type)) return Vec4d();
    if (i >= (int)chunkIDs[type].size() || i < 0) return Vec4d();
    return texture->getChunkUV( chunkIDs[type][i] );
}

void VRDistrict::addTexture(VRTexturePtr tex, string type) {
    Vec2i ID, pos;
    if (chunkIDs.count(type) == 0) ID[1] = chunkIDs.size();
    else { ID[1] = chunkIDs[type][0][1]; ID[0] = chunkIDs[type].size(); }
    pos = Vec2i(ID[0]*256, ID[1]*256);
    texture->add(tex, pos, ID);
    chunkIDs[type].push_back(ID);

    int Nmax = 1;
    for (auto c : chunkIDs) Nmax = max(Nmax, int(c.second.size()));

    b_mat->setTexture(texture, false);
    b_mat->setShaderParameter("chunkSize", Vec2f(1.0/Nmax, 1.0/chunkIDs.size()));
}

void VRDistrict::addTextures(string folder, string type) {
    for (auto f : openFolder(folder)) {
        auto tex = VRTexture::create();
        tex->read(folder + "/" + f);
        addTexture(tex, type);
    }
}

VRTextureMosaicPtr VRDistrict::getTexture() { return texture; }

void VRDistrict::addBuilding( VRPolygonPtr p, int stories, string housenumber, string street, string type ) {
    p->removeDoubles(0.1);
    if (p->size() < 3) return;
    if (p->isCCW()) p->reverseOrder();
    auto b = VRBuilding::create();
    b->setType(type);
    string ID = street+housenumber;
    if (ID == "" || buildings.count(ID)) {
        static int i = 0; i++;
        ID = "__placeholder__"+toString(i);
    }
    buildings[ID] = b;
    b->setWorld(world.lock());

    b->addFoundation(*p, 2.5);
    for (auto i=0; i<stories; i++) b->addFloor(*p, 2.5);
    b->addRoof(*p);
    b->computeGeometry(facades, roofs, ptr());

    auto toStringVector = [](const Vec3d& v) {
        vector<string> res;
        res.push_back( toString(v[0]) );
        res.push_back( toString(v[1]) );
        res.push_back( toString(v[2]) );
        return res;
    };

    auto o = ontology.lock();
    if (o) {
        auto bEnt = o->addEntity("building", "Building");
        bEnt->set("streetName", street);
        bEnt->set("houseNumber", housenumber);
        auto area = o->addEntity("area", "Area");
        auto perimeter = o->addEntity("perimeter", "Path");
        area->set("borders", perimeter->getName());
        bEnt->set("area", area->getName());

        for (auto pnt : p->get()) {
            auto node = o->addEntity("node", "Node");
            node->setVector("position", toStringVector(Vec3d(pnt[0],0,pnt[1])), "Position");
            auto nodeEntry = o->addEntity(name+"Entry", "NodeEntry");
            nodeEntry->set("path", perimeter->getName());
            nodeEntry->set("node", node->getName());
            nodeEntry->set("sign", "0");
            nodeEntry->setVector("direction", toStringVector(Vec3d()), "Direction");
            node->add("paths", nodeEntry->getName());
            perimeter->add("nodes", nodeEntry->getName());
        }
    }
#ifndef WITHOUT_BULLET
    auto geo = b->getCollisionShape();
    if (auto w = world.lock()) w->getPhysicsSystem()->add(geo, getID());
#endif
}

void VRDistrict::computeGeometry() {
    init();
    for (auto b : buildings) b.second->computeGeometry(facades, roofs, ptr());
}

void VRDistrict::remBuilding( string street, string housenumber ) {
    string ID = street+housenumber;
    if (!buildings.count(ID)) cout << "VRDistrict::remBuilding, Warning: building unknown with address '" << street << " " << housenumber << "'\n";
    buildings.erase(ID);
    computeGeometry();
}

void VRDistrict::clear() {
    buildings.clear();
    init();
}

string VRDistrict::matVShdr = GLSL(
varying vec4 vertex;
varying vec3 vnrm;
varying vec4 vtc1;
varying vec2 vtc2;
varying vec2 vtc3;
attribute vec4 osg_Vertex;
attribute vec3 osg_Normal;
attribute vec4 osg_Color;
attribute vec2 osg_MultiTexCoord0;
attribute vec2 osg_MultiTexCoord1;

void main( void ) {
    vnrm = gl_NormalMatrix * osg_Normal;
    vtc1 = osg_Color.xyzw;
    vtc2 = vec2(osg_MultiTexCoord0);
    vtc3 = vec2(osg_MultiTexCoord1);
    vertex = gl_ModelViewMatrix * osg_Vertex;
    gl_Position = gl_ModelViewProjectionMatrix * osg_Vertex;
}
);


string VRDistrict::matFShdr = GLSL(
varying vec4 vertex;
varying vec3 vnrm;
varying vec4 vtc1;
varying vec2 vtc2;
varying vec2 vtc3;
uniform sampler2D tex;
uniform vec2 chunkSize;

float padding = 0.025; // 0.05
vec4 color;
vec3 normal;

void applyLightning() {
	vec3 n = normal;
	vec3  light = normalize( gl_LightSource[0].position.xyz );// directional light
	float NdotL = max(dot( n, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot(n, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	gl_FragColor = ambient + diffuse + specular;
	//gl_FragColor = vec4(n*NdotL, 1.0);
}

vec2 modTC(vec2 tc) {
	vec2 res = tc.xy/chunkSize.xy;
	res -= floor(res.xy);
	res.xy *= chunkSize.xy;
	res.xy *= 1.0-2.0*padding;
	res.xy += padding*chunkSize.xy;
	if (res.x < padding*chunkSize.x) res.x = padding*chunkSize.x;
	if (res.x > (1.0-padding)*chunkSize.x) res.x = (1.0-padding)*chunkSize.x;
	if (res.y < padding*chunkSize.y) res.y = padding*chunkSize.y;
	if (res.y > (1.0-padding)*chunkSize.y) res.y = (1.0-padding)*chunkSize.y;
	return res;
}

float alphaFix(vec2 uv, float a) {
    vec2 p = 2.0*padding*chunkSize;
	if (uv.x < p.x)               return 0.0;
	if (uv.x > chunkSize.x - p.x) return 0.0;
	if (uv.y < p.y)               return 0.0;
	if (uv.y > chunkSize.y - p.y) return 0.0;
	return a;
}

void main( void ) {
	normal = normalize( vnrm );
	vec2 uv1 = modTC(vtc2);
	vec2 uv2 = modTC(vtc3);
	vec4 tex1 = texture2D(tex, uv1 + vtc1.xy);
	vec4 tex2 = texture2D(tex, uv2 + vtc1.zw);
	tex2[3] = alphaFix(uv2, tex2[3]);
	color = mix(tex1, tex2, tex2[3]);
	applyLightning();
}
);

string VRDistrict::matFDShdr = GLSL(
varying vec4 vertex;
varying vec3 vnrm;
varying vec4 vtc1;
varying vec2 vtc2;
varying vec2 vtc3;
uniform sampler2D tex;
uniform vec2 chunkSize;

float padding = 0.025; // 0.05
vec4 color;
vec3 normal;

vec2 modTC(vec2 tc) {
	vec2 res = tc.xy/chunkSize.xy;
	res -= floor(res.xy);
	res.xy *= chunkSize.xy;
	res.xy *= 1.0-2.0*padding;
	res.xy += padding*chunkSize.xy;
	if (res.x < padding*chunkSize.x) res.x = padding*chunkSize.x;
	if (res.x > (1.0-padding)*chunkSize.x) res.x = (1.0-padding)*chunkSize.x;
	if (res.y < padding*chunkSize.y) res.y = padding*chunkSize.y;
	if (res.y > (1.0-padding)*chunkSize.y) res.y = (1.0-padding)*chunkSize.y;
	return res;
}

float alphaFix(vec2 uv, float a) {
    vec2 p = 2.0*padding*chunkSize;
	if (uv.x < p.x)               return 0.0;
	if (uv.x > chunkSize.x - p.x) return 0.0;
	if (uv.y < p.y)               return 0.0;
	if (uv.y > chunkSize.y - p.y) return 0.0;
	return a;
}

void main( void ) {
	normal = normalize( vnrm );
	vec2 uv1 = modTC(vtc2);
	vec2 uv2 = modTC(vtc3);
	vec4 tex1 = texture2D(tex, uv1 + vtc1.xy);
	vec4 tex2 = texture2D(tex, uv2 + vtc1.zw);
	tex2[3] = alphaFix(uv2, tex2[3]);
	color = mix(tex1, tex2, tex2[3]);

    gl_FragData[0] = vec4(vertex.xyz/vertex.w, 1.0);
    gl_FragData[1] = vec4(normal, 1);
    gl_FragData[2] = color;
}
);













