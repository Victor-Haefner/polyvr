#include "VRWorldGenerator.h"
#include "GIS/OSMMap.h"
#include "terrain/VRPlanet.h"
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
    terrain->setWorld( ptr() );
    roads->setWorld( ptr() );
    nature->setWorld( ptr() );
}

void VRWorldGenerator::setPlanet(VRPlanetPtr p, Vec2d c) {
    coords = c;
    planet = p;
    terrain->setWorld( ptr() );
    roads->setWorld( ptr() );
    nature->setWorld( ptr() );
}

VROntologyPtr VRWorldGenerator::getOntology() { return ontology; }
VRRoadNetworkPtr VRWorldGenerator::getRoadNetwork() { return roads; }
VRObjectManagerPtr VRWorldGenerator::getAssetManager() { return assets; }
VRTerrainPtr VRWorldGenerator::getTerrain() { return terrain; }
VRNaturePtr VRWorldGenerator::getNature() { return nature; }
VRPlanetPtr VRWorldGenerator::getPlanet() { return planet; }

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
    terrain->setWorld( ptr() );
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

void VRWorldGenerator::addOSMMap(string path) {
    osmMap = OSMMap::loadMap(path);

    // road network data

    for (auto wayItr : osmMap->getWays()) {
        auto& way = wayItr.second;
        auto p = way->polygon;
        vector<Vec3d> points;

        for (auto pnt : p.get()) {
            Vec3d pos = Vec3d( planet->fromLatLongPosition(pnt[1], pnt[0], true) );
            points.push_back(pos);
        }

        cout << " tags: ";
        for (auto tag : way->tags) {
            if (tag.first == "highway") { // TODO: prototype
                vector<Vec3d> norms;
                for (int i=0; i<points.size(); i++) {
                    Vec3d n;
                    if (i == 0) n = points[1]-points[0];
                    else if (i == points.size()-1) n = points[points.size()-1]-points[points.size()-2];
                    else n = points[i+1] - points[i-1]; // (points[i] - points[i-1]) + (points[i+1] - points[i]);
                    n.normalize();
                    norms.push_back(n);
                }

                VREntityPtr node1, node2;
                for (int i=1; i<points.size(); i++) {
                    auto p1 = points[i-1];
                    auto p2 = points[i];
                    node1 = node2;
                    if (i == 1) node1 = roads->addNode(p1, true);
                    node2 = roads->addNode(p2, true);
                    Vec3d norm1 = norms[i-1];
                    Vec3d norm2 = norms[i];
                    //roads->addRoad("someRoad", "highway", node1, node2, norm1, norm2, 2);
                }
            }
            cout << "   " << tag.first << ":" << tag.second;
        }
        cout << endl;
    }

    // -------------------- project OSM polygons on texture
    /*auto dim = tex->getSize();
    VRTextureGenerator tg;
    tg.setSize(dim, true);

    //for (auto tag : polygons) cout << "polygon tag: " << tag.first << endl;

    auto drawPolygons = [&](string tag, Color4f col) {
        if (!polygons.count(tag)) {
            //cout << "\ndrawPolygons: tag '" << tag << "' not found!" << endl;
            return;
        }

        for (auto p : polygons[tag]) {
            p->scale( Vec3d(1.0/size[0], 1, 1.0/size[1]) );
            p->translate( Vec3d(0.5,0,0.5) );
            tg.drawPolygon( p, col );
        }
    };

    drawPolygons("natural", Color4f(0,1,0,1));
    drawPolygons("water", Color4f(0.2,0.4,1,1));
    drawPolygons("industrial", Color4f(0.2,0.2,0.2,1));
    VRTexturePtr t = tg.compose(0);

    // ----------------------- combine OSM texture with heightmap
    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim[1]; j++) {
            Vec3i pixK = Vec3i(i,j,0);
            double h = tex->getPixel(pixK)[3];
            auto pix = Vec2d(i*1.0/(dim[0]-1), j*1.0/(dim[1]-1));
            //if (tgPolygon->isInside(pix)) h = 14;
            Color4f col = t->getPixel(pixK);
            col[3] = h;
            t->setPixel(pixK, col);
        }
    }
    setMap(t);*/
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



