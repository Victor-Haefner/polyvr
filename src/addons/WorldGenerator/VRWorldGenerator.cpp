#include "VRWorldGenerator.h"
#include "GIS/OSMMap.h"
#include "terrain/VRPlanet.h"
#include "roads/VRRoad.h"
#include "roads/VRRoadNetwork.h"
#include "nature/VRNature.h"
#include "terrain/VRTerrain.h"
#include "buildings/VRDistrict.h"
#include "core/objects/VRTransform.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRObjectManager.h"
#include "core/utils/toString.h"
#include "addons/Semantics/Reasoning/VROntology.h"

#define GLSL(shader) #shader

using namespace OSG;

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
    nature->simpleInit(20, 20);

    district = VRDistrict::create();
    district->setWorld( ptr() );
    addChild(district);
}

void VRWorldGenerator::addOSMMap(string path) {
    osmMap = OSMMap::loadMap(path);
    processOSMMap();
}

void VRWorldGenerator::processOSMMap() {
    struct Node {
        OSMNodePtr n;
        VREntityPtr e;
        Vec3d p;
    };

    map<string, Node> graphNodes;

    auto wayToPolygon = [&](OSMWayPtr& way) {
        auto poly = VRPolygon::create();
        for (auto ps : way->polygon.get()) {
            auto p = planet->fromLatLongPosition(ps[1], ps[0], true);
            poly->addPoint( Vec2d(p[0], p[2]) );
        }
        if (!poly->isCCW()) poly->reverseOrder();
        return poly;
    };

    auto getDir  = [&](OSMNodePtr n) {
        float a = planet->toRad( toFloat( n->tags["direction"] ) ); // angle
        return Vec3d(sin(a), 0, -cos(a));
    };

    auto addRoad = [&](OSMWayPtr& way, string tag, float Width, bool pedestrian) {
        if (way->nodes.size() < 2) return;
        vector<VREntityPtr> nodes;
        vector<Vec3d> norms;

        //auto pos  = [&](int i) { return graphNodes[way->nodes[i]].p; };
        //auto getNode = [&](int i) { return graphNodes[way->nodes[i]].e; };
        auto has  = [&](const string& tag) { return way->tags.count(tag) > 0; };

        auto addPathData = [&](VREntityPtr node, Vec3d norm) {
            norm.normalize();
            nodes.push_back(node);
            norms.push_back(norm);
        };

        float height = has("height") && has("embankment") ? toFloat(way->tags["height"]) : 0;

        for (uint i=1; i<way->nodes.size(); i++) { // TODO: consider using direction tag in OSM
            auto& n1 = graphNodes[ way->nodes[i-1] ];
            auto& n2 = graphNodes[ way->nodes[i  ] ];
            auto& n3 = graphNodes[ way->nodes[(i+1)%way->nodes.size()] ];

            if (i == 1) { // first
                if (!n1.e) n1.e = roads->addNode(n1.p, true, height);
                addPathData(n1.e, n1.n->tags.count("direction") ? getDir(n1.n) : n2.p - n1.p);
            }

            //auto node = roads->addNode((n1.p+n2.p)*0.5, true);
            //addPathData(node, n2.p - n1.p);

            if (i == way->nodes.size()-1) { // last
                if (!n2.e) n2.e = roads->addNode(n2.p, true, height);
                addPathData(n2.e, n2.n->tags.count("direction") ? getDir(n2.n) : n2.p - n1.p);
            } else if (n2.n->Nways > 1 || true) { // intersection node, add it!
                if (!n2.e) n2.e = roads->addNode(n2.p, true, height);
                addPathData(n2.e, n2.n->tags.count("direction") ? getDir(n2.n) : n3.p - n1.p);
            }
        }

        int NlanesRight = has("lanes:forward") ? toInt( way->tags["lanes:forward"] ) : has("lanes:forwards") ? toInt( way->tags["lanes:forwards"] ) : 0;
        int NlanesLeft = has("lanes:backward") ? toInt( way->tags["lanes:backward"] ) : 0;
        if (NlanesRight == 0 && NlanesLeft == 0) NlanesRight = has("lanes") ? toInt( way->tags["lanes"] ) : 1;
        //cout << endl << has("lanes") << " hasForw " << has("lanes:forward") << " hasBack " << has("lanes:backward") << " Nright " << NlanesRight << " Nleft " << NlanesLeft << endl;

        for (uint i=1; i<nodes.size(); i++) {
            auto road = roads->addRoad("someRoad", tag, nodes[i-1], nodes[i], norms[i-1], norms[i], 0);
            for (int l=0; l < NlanesRight; l++) road->addLane(1, Width, pedestrian);
            for (int l=0; l < NlanesLeft; l++) road->addLane(-1, Width, pedestrian);
        }
    };

    auto addBuilding = [&](OSMWayPtr& way) {
        auto has  = [&](const string& tag) { return way->tags.count(tag) > 0; };

        int lvls = 4;
        if (has("building:levels")) lvls = toInt( way->tags["building:levels"] );
        district->addBuilding( *wayToPolygon(way), lvls );
    };

    int i=0;
    for (auto wayItr : osmMap->getWays()) {
        i++;
        //if (i != 105 && i != 271 && i != 299) continue; // 146, 105, 271, 298, 299
        //if (i != 271 && i != 299) continue; // 146, 105, 271, 298, 299
        auto& way = wayItr.second;
        for (auto pID : way->nodes) {
            if (graphNodes.count(pID)) continue;
            Node n;
            n.n = osmMap->getNode(pID);
            n.p = planet->fromLatLongPosition(n.n->lat, n.n->lon, true);
            graphNodes[pID] = n;
        }

        cout << "tags: ";
        for (auto tag : way->tags) {
            if (tag.first == "highway") {
                if (tag.second == "footway") { addRoad(way, tag.second, 1, true); continue; }
                addRoad(way, tag.second, 4, false); // default road
                continue;
            }

            if (tag.first == "building") { addBuilding(way); continue; }

            if (tag.first == "barrier") {
                if (tag.second == "rail_guard") ; // TODO: port from python prototype
                continue;
            }

            if (tag.first == "natural") {
                if (tag.second == "wood") {
                    //nature->addGrassPatch( wayToPolygon(way), 0, 1, 1 );
                }
                if (tag.second == "grassland") ; // TODO
                continue;
            }

            if (tag.first == "leisure") {
                if (tag.second == "park") {
                    //static int b = 0;
                    //if (b < 3) {
                        auto poly = wayToPolygon(way);
                        //cout << " addWoods " << poly->computeArea() << endl;
                        nature->addGrassPatch( poly, 0, 1, 0 );
                        //b++;
                    //}
                    //nature->addGrassPatch( wayToPolygon(way), 0, 1, 1 );
                }
                continue;
            }
            cout << " " << tag.first << " : " << tag.second;
        }
        cout << endl;
    }

    for (auto nodeItr : osmMap->getNodes()) {
        auto& node = nodeItr.second;
        Vec3d pos = planet->fromLatLongPosition(node->lat, node->lon, true);
        Vec3d dir = getDir(node);
        if (terrain) terrain->elevatePoint(pos);
        for (auto tag : node->tags) {
            if (tag.first == "natural") {
                if (tag.second == "tree") {
                    auto t = nature->createRandomTree(pos);
                    nature->addTree(t, 0, 0);
                }
                continue;
            }
            if (tag.first == "traffic_sign:training_ground") {
                auto a = assets->copy(tag.second, pose::create(pos, dir), false);
                //cout << " add asset, sign " << tag.second << "  " << a << endl;
            }
        }
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

void VRWorldGenerator::reloadOSMMap() {
    clear();
    osmMap->reload();
    processOSMMap();
    roads->compute();
}

void VRWorldGenerator::clear() {
    osmMap->clear();
    roads->clear();
    district->clear();
}


// textured asset material

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



// asset material

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



