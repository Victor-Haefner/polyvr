#include "VRWorldGenerator.h"
#include "GIS/OSMMap.h"
#include "terrain/VRPlanet.h"
#include "roads/VRAsphalt.h"
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
#include "core/math/path.h"
#include "core/math/triangulator.h"
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

void VRWorldGenerator::addOSMMap(string path, double subN, double subE, double subSize) {
    osmMap = OSMMap::loadMap(path);
    processOSMMap(subN, subE, subSize);
}

void VRWorldGenerator::processOSMMap(double subN, double subE, double subSize) {
    struct Node {
        OSMNodePtr n;
        VREntityPtr e;
        Vec3d p;
    };

    map<string, Node> graphNodes;
    map<string, VRRoadPtr> RoadEntities;

    auto wayToPolygon = [&](OSMWayPtr& way) {
        auto poly = VRPolygon::create();
        for (auto ps : way->polygon.get()) {
            auto p = planet->fromLatLongPosition(ps[1], ps[0], true);
            poly->addPoint( Vec2d(p[0], p[2]) );
        }
        if (!poly->isCCW()) poly->reverseOrder();
        return poly;
    };

    auto wayToPath = [&](OSMWayPtr& way, int N) -> pathPtr {
        auto path = path::create();
        vector<Vec3d> pos;
        for (auto nID : way->nodes) {
            auto n = osmMap->getNode(nID);
            Vec3d p = planet->fromLatLongPosition(n->lat, n->lon, true);
            p[1] = n->hasTag("height") ? toFloat(n->tags["height"]) : 0;
            pos.push_back( p );
            //path->addPoint( pose( p ) );
        }

        auto addPnt = [&](Vec3d p, Vec3d d) {
            //d.normalize(); // TODO: necessary because of projectTangent, can be optimized!
            if (terrain) {
                terrain->elevatePoint(p,p[1]);
                terrain->projectTangent(d, p);
            } else d.normalize();
            path->addPoint( Pose( p, d ) );
        };

        if (pos.size() < 2) return 0;
        if (pos.size() == 2) {
            Vec3d d = pos[1] - pos[0];
            addPnt(pos[0], d);
            addPnt(pos[1], d);
        }

        for (uint i=1; i<pos.size()-1; i++) {
            auto& p1 = pos[i-1];
            auto& p2 = pos[i];
            auto& p3 = pos[i+1];

            if (i == 1) addPnt(p1, p2-p1);
            addPnt(p2, p3-p1);
            if (i == pos.size()-2) addPnt(p3, p3-p2);
        }
        path->compute(N);
        return path;
    };

    auto embSlopePath = [&](OSMWayPtr& way, int N) {
        auto path = path::create();
        vector<Vec3d> pos;
        for (auto nID : way->nodes) {
            auto n = osmMap->getNode(nID);
            Vec3d p = planet->fromLatLongPosition(n->lat, n->lon, true);
            pos.push_back( p );
        }

        auto addPnt = [&](Vec3d p, Vec3d d) {
            if (terrain) {
                terrain->elevatePoint(p);
                terrain->projectTangent(d, p);
            }
            //d[1] = 0;
            d.normalize();
            path->addPoint( Pose( p, d ) );
        };

        if (pos.size() == 2) {
            Vec3d d = pos[1] - pos[0];
            addPnt(pos[0], d);
            addPnt(pos[1], d);
        }

        for (uint i=1; i<pos.size()-1; i++) {
            auto& p1 = pos[i-1];
            auto& p2 = pos[i];
            auto& p3 = pos[i+1];

            if (i == 1) addPnt(p1, p2-p1);
            addPnt(p2, p3-p1);
            if (i == pos.size()-2) addPnt(p3, p3-p2);
        }
        path->compute(N);
        return path;
    };

    auto getDir = [&](OSMNodePtr n) {
        float a = 0;
        if (n->hasTag("direction")) a = planet->toRad( toFloat( n->tags["direction"] ) ); // angle
        return Vec3d(sin(a), 0, -cos(a));
    };

    auto addRoad = [&](OSMWayPtr& way, string tag, float width, bool pedestrian) {
        if (way->nodes.size() < 2) return;
        string name = "road";
        if (way->hasTag("name")) name = way->tags["name"];

        vector<VREntityPtr> nodes;
        vector<Vec3d> norms;

        auto getInt = [&](string tag, int def) { return way->hasTag(tag) ? toInt( way->tags[tag] ) : def; };
        //auto getFloat = [&](string tag, float def) { return way->hasTag(tag) ? toFloat( way->tags[tag] ) : def; };
        auto getString = [&](string tag, string def) { return way->hasTag(tag) ? way->tags[tag] : def; };

        auto addPathData = [&](VREntityPtr node, Vec3d pos, Vec3d norm) {
            if (terrain) terrain->projectTangent(norm, pos);
            else norm.normalize();
            nodes.push_back(node);
            norms.push_back(norm);
        };

        for (uint i=1; i<way->nodes.size(); i++) {
            auto& n1 = graphNodes[ way->nodes[i-1] ];
            auto& n2 = graphNodes[ way->nodes[i  ] ];
            auto& n3 = graphNodes[ way->nodes[(i+1)%way->nodes.size()] ];

            if (i == 1) { // first
                if (!n1.e) n1.e = roads->addNode(n1.p, true, 0);
                addPathData(n1.e, n1.p, n1.n->tags.count("direction") ? getDir(n1.n) : n2.p - n1.p);
            }

            if (!n2.e) n2.e = roads->addNode(n2.p, true, 0);
            Vec3d n = (i == way->nodes.size()-1) ? n2.p - n1.p : n3.p - n1.p;
            addPathData(n2.e, n2.p, n2.n->tags.count("direction") ? getDir(n2.n) : n);
        }

        // compute number of lanes in each direction
        int NlanesRight = way->hasTag("lanes:forward") ? toInt( way->tags["lanes:forward"] ) : 0;
        int NlanesLeft = way->hasTag("lanes:backward") ? toInt( way->tags["lanes:backward"] ) : 0;
        if (NlanesRight == 0 && NlanesLeft == 0) NlanesRight = way->hasTag("lanes") ? toInt( way->tags["lanes"] ) : 1;
        //cout << endl << way->hasTag("lanes") << " hasForw " << way->hasTag("lanes:forward") << " hasBack " << way->hasTag("lanes:backward") << " Nright " << NlanesRight << " Nleft " << NlanesLeft << endl;

        bool hasPLaneR = bool(way->hasTag("parking:lane:right"));
        bool hasPLaneL = bool(way->hasTag("parking:lane:left"));

        // compute parameters for the lanes
        vector<float> widths;
        if (way->hasTag("width:lanes")) {
            auto data = splitString( way->tags["width:lanes"], '|' );
            for (auto d : data) widths.push_back(toFloat(d));
        } else if (way->hasTag("width:lanes:forward") || way->hasTag("width:lanes:backward")) {
            auto dataRight = splitString( way->tags["width:lanes:forward"], '|' );
            auto dataLeft = splitString( way->tags["width:lanes:backward"], '|' );
            for (auto d : dataRight) widths.push_back(toFloat(d));
            for (auto d : dataLeft) widths.push_back(toFloat(d));
        } else if (way->hasTag("width")) widths = vector<float>( NlanesRight+NlanesLeft+hasPLaneR+hasPLaneL, toFloat(way->tags["width"]) );
        else widths = vector<float>( NlanesRight+NlanesLeft+hasPLaneR+hasPLaneL, width );

        // create road and lane entities
        auto road = roads->addLongRoad(name, tag, nodes, norms, 0);
        if (hasPLaneR) road->addParkingLane(1, widths[NlanesRight], getInt("parking:lane:right:capacity", 0), getString("parking:lane:right", ""));
        for (int l=0; l < NlanesRight; l++) road->addLane(1, widths[NlanesRight-1-l], pedestrian);
        for (int l=0; l < NlanesLeft; l++) road->addLane(-1, widths[NlanesRight+hasPLaneR+l], pedestrian);
        if (hasPLaneL) road->addParkingLane(-1, widths[NlanesRight+NlanesLeft+hasPLaneR], getInt("parking:lane:left:capacity", 0), getString("parking:lane:left", ""));
        RoadEntities[way->id] = road;
    };

    auto addBuilding = [&](OSMWayPtr& way) {
        int lvls = 4;
        if (way->hasTag("building:levels")) lvls = toInt( way->tags["building:levels"] );
        district->addBuilding( *wayToPolygon(way), lvls );
    };

    auto nodeInSubarea = [&](OSMNodePtr node) {
        if (!node) return false;
        if (subSize < 0) return true;
        //cout << " nodeInSubarea " << node->lon << "  " << subN << "    " << node->lat << "   " << subE << endl;
        return bool(abs(node->lat-subN) < subSize && abs(node->lon-subE) < subSize);
    };
    auto wayInSubarea = [&](OSMWayPtr way) { if (!way) return false; if (subSize < 0) return true; for (auto n : way->nodes) if (!nodeInSubarea(osmMap->getNode(n))) return false; return true; };
    auto relInSubarea = [&](OSMRelationPtr& rel) { if (!rel) return false; if (subSize < 0) return true; for (auto n : rel->nodes) if (!nodeInSubarea(osmMap->getNode(n))) return false; for (auto w : rel->ways) if (!wayInSubarea(osmMap->getWay(w))) return false; return true; };

    for (auto relItr : osmMap->getRelations()) {
        auto& rel = relItr.second;
        if (!relInSubarea(rel)) continue;
        if (rel->hasTag("embankment") && rel->ways.size() == 2) { // expect two parallel ways
            auto w1 = osmMap->getWay(rel->ways[0]);
            auto w2 = osmMap->getWay(rel->ways[1]);
            auto p1 = wayToPath(w1, 8);
            auto p2 = wayToPath(w2, 8);
            auto p3 = embSlopePath(w1, 8);
            auto p4 = embSlopePath(w2, 8);
            terrain->addEmbankment(rel->id, p1, p2, p3, p4);
        }
    }

    for (auto wayItr : osmMap->getWays()) { // use way->id to filter for debugging!
        auto& way = wayItr.second;
        if (!wayInSubarea(way)) continue;
        for (auto pID : way->nodes) {
            if (graphNodes.count(pID)) continue;
            Node n;
            n.n = osmMap->getNode(pID);
            n.p = planet->fromLatLongPosition(n.n->lat, n.n->lon, true);
            graphNodes[pID] = n;
        }

        for (auto tag : way->tags) {
            if (tag.first == "highway") {
                if (tag.second == "footway") { addRoad(way, tag.second, 1, true); continue; }
                addRoad(way, tag.second, 4, false); // default road
                continue;
            }

            if (tag.first == "building") { addBuilding(way); continue; }

            if (tag.first == "barrier") {
                if (tag.second == "guard_rail") {
                    //cout << "VRWorldGenerator::processOSMMap guard_rail " << endl;
                    float h = way->hasTag("height") ? toFloat( way->tags["height"] ) : 0.5;
                    roads->addGuardRail( wayToPath(way, 8), h );
                }
                if (tag.second == "kerb") {
                    float h = way->hasTag("height") ? toFloat( way->tags["height"] ) : 0.2;
                    roads->addKirb( wayToPolygon(way), h );
                }
                continue;
            }

            if (tag.first == "natural") {
                //if (tag.second == "woods") nature->addWoods( wayToPolygon(way), 1);
                if (tag.second == "scrub") nature->addScrub( wayToPolygon(way), 1);
                //if (tag.second == "grassland") nature->addGrassPatch( wayToPolygon(way), 0, 1, 0);
                continue;
            }

            if (tag.first == "landuse") { // TODO
                auto patch = VRGeometry::create("patch");
                auto poly = wayToPolygon(way);
                if (poly->size() == 0) continue;
                for (auto p : poly->gridSplit(1)) {
                    if (terrain) terrain->elevatePolygon(p, 0.03, false);
                    Triangulator tri;
                    tri.add(*p);
                    patch->merge( tri.compute() );
                }
                patch->setMaterial(roads->getMaterial());
                patch->setPositionalTexCoords(1.0, 0, Vec3i(0,2,1));
                addChild(patch);
            }
        }
    }

    for (auto nodeItr : osmMap->getNodes()) {
        auto& node = nodeItr.second;
        if (!nodeInSubarea(node)) continue;
        Vec3d pos = planet->fromLatLongPosition(node->lat, node->lon, true);
        Vec3d dir = getDir(node);
        if (terrain) terrain->elevatePoint(pos);
        for (auto tag : node->tags) {
            if (tag.first == "natural") {
                if (tag.second == "tree") nature->createRandomTree(pos);
                continue;
            }

            if (tag.first == "traffic_sign:training_ground") {
                auto signEnt = ontology->addEntity("sign", "Sign");
                signEnt->set("type", tag.second);
                signEnt->setVec3("position", pos, "Position");
                signEnt->setVec3("direction", dir, "Direction");
                for (auto way : node->ways) {
                    if (!RoadEntities.count(way)) continue;
                    auto roadEnt = RoadEntities[node->ways[0]]->getEntity();
                    roadEnt->add("signs",signEnt->getName());
                    signEnt->set("road",roadEnt->getName());
                }
            }

            if (tag.first == "surveillance:type") {
                if (tag.second == "camera") {
                    assets->copy("Camera", Pose::create(pos, dir), false);
                }
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

void VRWorldGenerator::reloadOSMMap(double subN, double subE, double subSize) {
    clear();
    osmMap->reload();
    processOSMMap(subN, subE, subSize);
    roads->compute();
}

void VRWorldGenerator::clear() {
    osmMap->clear();
    roads->clear();
    district->clear();
    assets->clear(false);
    terrain->clear();
    nature->clear();
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



