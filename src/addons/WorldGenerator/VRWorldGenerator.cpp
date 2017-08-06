#include "VRWorldGenerator.h"
#include "roads/VRRoadNetwork.h"
#include "nature/VRNature.h"
#include "terrain/VRTerrain.h"
#include "core/objects/VRTransform.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRObjectManager.h"

#define GLSL(shader) #shader

using namespace OSG;

string assetTexMatVShdr;
string assetTexMatFShdr;
string assetMatVShdr;
string assetMatFShdr;

VRWorldGenerator::VRWorldGenerator() : VRTransform("WorldGenerator") {}
VRWorldGenerator::~VRWorldGenerator() {}

VRWorldGeneratorPtr VRWorldGenerator::create() {
    auto wg = VRWorldGeneratorPtr( new VRWorldGenerator() );
    wg->init();
    return wg;
}

VRWorldGeneratorPtr VRWorldGenerator::ptr() { return dynamic_pointer_cast<VRWorldGenerator>( shared_from_this() ); }

void VRWorldGenerator::setOntology(VROntologyPtr o) {
    ontology = o;
    roads->setWorld( ptr() );
    nature->setWorld( ptr() );
}

VROntologyPtr VRWorldGenerator::getOntology() { return ontology; }
VRRoadNetworkPtr VRWorldGenerator::getRoadNetwork() { return roads; }
VRObjectManagerPtr VRWorldGenerator::getAssetManager() { return assets; }
VRTerrainPtr VRWorldGenerator::getTerrain() { return terrain; }
VRNaturePtr VRWorldGenerator::getNature() { return nature; }

void VRWorldGenerator::addMaterial( string name, VRMaterialPtr mat ) { materials[name] = mat; }
void VRWorldGenerator::addAsset( string name, VRTransformPtr geo ) {
    for (auto o : geo->getChildren(true, "Geometry")) {
        auto g = dynamic_pointer_cast<VRGeometry>(o);
        auto m = getMaterial("phong");
        if (g->getMaterial()->getTexture()) continue; // m = getMaterial("phongTex"); // TODO
        auto c = g->getMaterial()->getDiffuse();
        VRGeoData data(g);
        data.addVertexColors(c);
        g->setMaterial(m);
    }
    assets->addTemplate(geo, name);
}

VRMaterialPtr VRWorldGenerator::getMaterial(string name) {
    if (!materials.count(name)) return 0;
    return materials[name];
}

void VRWorldGenerator::init() {
    auto addMat = [&](string name, string vp, string fp) {
        auto mat = VRMaterial::create(name);
        mat->setVertexShader(vp, name+"VS");
        mat->setFragmentShader(fp, name+"FS");
        addMaterial(name, mat);
    };

    addMat("phong", assetMatVShdr, assetMatFShdr);
    addMat("phongTex", assetTexMatVShdr, assetTexMatFShdr);

    terrain = VRTerrain::create();
    addChild(terrain);

    roads = VRRoadNetwork::create();
    roads->setWorld( ptr() );
    addChild(roads);

    assets = VRObjectManager::create();
    addChild(assets);
    nature = VRNature::create();
    nature->setWorld( ptr() );
    addChild(nature);
}



// asset material

string VRWorldGenerator::assetTexMatVShdr = GLSL(
varying vec3 vnrm;
varying vec3 vcol;
varying vec2 vtcs;
varying vec3 vpos;
attribute vec4 osg_Vertex;
attribute vec3 osg_Normal;
attribute vec4 osg_Color;
attribute vec2 osg_MultiTexCoord0;

void main( void ) {
	vnrm = normalize( gl_NormalMatrix * osg_Normal );
	vcol = osg_Color.xyz;
	vtcs = osg_MultiTexCoord0;
    vpos = osg_Vertex.xyz;
    gl_Position = gl_ModelViewProjectionMatrix * osg_Vertex;
}
);


string VRWorldGenerator::assetTexMatFShdr = GLSL(
uniform sampler2D tex;
varying vec2 vtcs;
varying vec3 vnrm;
varying vec3 vcol;
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
}

void main( void ) {
	normal = vnrm;
	color = texture2D(tex,vtcs);
	if (color.a < 0.3) discard;
	applyLightning();
}
);



// textured asset material

string VRWorldGenerator::assetMatVShdr = GLSL(
varying vec3 vnrm;
varying vec3 vcol;
varying vec3 vpos;
attribute vec4 osg_Vertex;
attribute vec3 osg_Normal;
attribute vec4 osg_Color;

void main( void ) {
	vnrm = normalize( gl_NormalMatrix * osg_Normal );
	vcol = osg_Color.xyz;
    vpos = osg_Vertex.xyz;
    gl_Position = gl_ModelViewProjectionMatrix * osg_Vertex;
}
);


string VRWorldGenerator::assetMatFShdr = GLSL(
varying vec3 vnrm;
varying vec3 vcol;
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
}

void main( void ) {
	normal = vnrm;
	color = vec4(vcol, 1.0);
	if (color.a < 0.3) discard;
	applyLightning();
}
);



