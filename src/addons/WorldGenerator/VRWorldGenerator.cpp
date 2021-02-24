#include "VRWorldGenerator.h"
#include "GIS/OSMMap.h"
#include "terrain/VRPlanet.h"
#include "roads/VRAsphalt.h"
#include "roads/VRRoad.h"
#include "roads/VRRoadNetwork.h"
#include "roads/VRTrafficSigns.h"
#include "nature/VRNature.h"
#include "terrain/VRTerrain.h"
#include "buildings/VRDistrict.h"
#include "core/objects/VRTransform.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRStroke.h"
#ifndef WITHOUT_BULLET
#include "core/objects/geometry/VRSpatialCollisionManager.h"
#endif
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRLodTree.h"
#include "core/scene/VRObjectManager.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/utils/zipper/filesystem.h"
#include "core/math/path.h"
#ifndef WITHOUT_GLU_TESS
#include "core/math/triangulator.h"
#endif
#include "addons/Semantics/Reasoning/VROntology.h"
#ifndef WITHOUT_GDAL
#include "core/scene/import/GIS/VRGDAL.h"
#endif
#include "core/objects/material/VRTexture.h"

#define GLSL(shader) #shader

using namespace OSG;


VRWorldGenerator::VRWorldGenerator() : VRTransform("WorldGenerator") {}

VRWorldGenerator::~VRWorldGenerator() {}

VRWorldGeneratorPtr VRWorldGenerator::create() {
    auto wg = VRWorldGeneratorPtr( new VRWorldGenerator() );
    wg->init();
    return wg;
}

VRWorldGeneratorPtr VRWorldGenerator::create(int meta) {
    if (meta == 0) {
        auto wg = VRWorldGeneratorPtr( new VRWorldGenerator() );
        wg->init();
        return wg;
    }
    if ( meta == 1 ) {
        auto wg = VRWorldGeneratorPtr( new VRWorldGenerator() );
        wg->initMinimum();
        return wg;
    }
    return 0;
}

VRWorldGeneratorPtr VRWorldGenerator::ptr() { return dynamic_pointer_cast<VRWorldGenerator>( shared_from_this() ); }

VRSpatialCollisionManagerPtr VRWorldGenerator::getPhysicsSystem() { return collisionShape; }

void VRWorldGenerator::setupPhysics() {
    /*auto c1 = nature->getCollisionObject();
    auto c2 = roads->getAssetCollisionObject();
    //collisionShape->add(c1);
    collisionShape->add(c2);*/
}

void VRWorldGenerator::updatePhysics(Boundingbox box) {
#ifndef WITHOUT_BULLET
    collisionShape->localize(box);
#endif
}

void VRWorldGenerator::setOntology(VROntologyPtr o) {
    ontology = o;
    for (auto terrain:terrains) terrain->setWorld( ptr() );
    roads->setWorld( ptr() );
    nature->setWorld( ptr() );
    district->setWorld( ptr() );
}

void VRWorldGenerator::setPlanet(VRPlanetPtr p, Vec2d c) {
    coords = c;
    planet = p;
    for (auto terrain:terrains) terrain->setWorld( ptr() );
    if (metaLVL == 0) roads->setWorld( ptr() );
    if (metaLVL == 0) nature->setWorld( ptr() );
    if (metaLVL == 0) district->setWorld( ptr() );
}

VROntologyPtr VRWorldGenerator::getOntology() { return ontology; }
VRRoadNetworkPtr VRWorldGenerator::getRoadNetwork() { return roads; }
VRTrafficSignsPtr VRWorldGenerator::getTrafficSigns() { return trafficSigns; }
VRObjectManagerPtr VRWorldGenerator::getAssetManager() { return assets; }
VRTerrainPtr VRWorldGenerator::getTerrain() { return terrains[0]; }
VRNaturePtr VRWorldGenerator::getNature() { return nature; }
VRPlanetPtr VRWorldGenerator::getPlanet() { return planet; }
VRDistrictPtr VRWorldGenerator::getDistrict() { return district; }
VRLodTreePtr VRWorldGenerator::getLodTree() { return lodTree; }

void VRWorldGenerator::addMaterial( string name, VRMaterialPtr mat ) { materials[name] = mat; }
void VRWorldGenerator::addAsset( string name, VRTransformPtr geo ) {
    if (!geo) return;
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

VRGeometryPtr VRWorldGenerator::getMiscArea(VREntityPtr mEnt){
    if (miscAreaByEnt.count(mEnt)) return miscAreaByEnt[mEnt];
    return 0;
}

void VRWorldGenerator::init() {
    metaLVL = 0;
    auto addMat = [&](string name, int texDim) {
        auto mat = VRMaterial::create(name);
        mat->setDefaultVertexShader();
        string fp = mat->constructShaderFP(0, false, texDim);
        string dfp = mat->constructShaderFP(0, true, texDim);

        //mat->setVertexShader(vp, name+"VS");
        mat->setFragmentShader(fp, name+"FS");
        mat->setFragmentShader(dfp, name+"DFS", true);
        addMaterial(name, mat);
    };
    lodTree = VRLodTree::create(name, 5);
    addChild(lodTree);

#ifndef WITHOUT_BULLET
    collisionShape = VRSpatialCollisionManager::create(12);
    addChild(collisionShape);
#endif

    addMat("phong", 0);
    addMat("phongTex", 2);

    setupLOD(4);
    auto terrain = VRTerrain::create();
    terrains.push_back(terrain);
    terrain->setWorld( ptr() );
    lodLevels[0]->addChild(terrain);

    roads = VRRoadNetwork::create();
    roads->setWorld( ptr() );
    lodLevels[0]->addChild(roads);

    trafficSigns = VRTrafficSigns::create();
    trafficSigns->setWorld( ptr() );
    lodLevels[0]->addChild(trafficSigns);

    assets = VRObjectManager::create();
    addChild(assets);

    nature = VRNature::create();
    nature->setWorld( ptr() );
    addChild(nature);
    //nature->simpleInit(10, 5);

    district = VRDistrict::create();
    district->setWorld( ptr() );
    addChild(district);
}

void VRWorldGenerator::initMinimum() {
    metaLVL = 1;
    auto addMat = [&](string name, int texDim) {
        auto mat = VRMaterial::create(name);
        mat->setDefaultVertexShader();
        string fp = mat->constructShaderFP(0, false, texDim);
        string dfp = mat->constructShaderFP(0, true, texDim);

        //mat->setVertexShader(vp, name+"VS");
        mat->setFragmentShader(fp, name+"FS");
        mat->setFragmentShader(dfp, name+"DFS", true);
        addMaterial(name, mat);
    };

    addMat("phong", 0);
    addMat("phongTex", 2);

    setupLOD(4);
    auto terrain = VRTerrain::create();
    terrains.push_back(terrain);
    terrain->setWorld( ptr() );
    lodLevels[1]->addChild(terrain);

    assets = VRObjectManager::create();
    addChild(assets);
}

Vec2d VRWorldGenerator::getPlanetCoords() { return coords; }

void VRWorldGenerator::addOSMMap(string path, double subN, double subE, double subSize) {
    osmMap = OSMMap::loadMap(path);
    processOSMMap(subN, subE, subSize);
}

void VRWorldGenerator::addGML(string path, int EPSG_Code) {
    auto newgmlMap = OSMMap::create();
    newgmlMap->readGML(path, EPSG_Code);
    gmlMap = newgmlMap;
    processGMLfromOSM();
}

void VRWorldGenerator::readOSMMap(string path){
    osmMap = OSMMap::loadMap(path);
}

OSMMapPtr VRWorldGenerator::getOSMMap() { return osmMap; }

OSMMapPtr VRWorldGenerator::getGMLMap() { return gmlMap; }

void VRWorldGenerator::addTerrainsToLOD(){
    cout << "VRWorldGenerator::addTerrainsToLOD" << endl;
    //auto nLevel = lodLevels.size();
    auto nTerrains = terrains.size();
    for (unsigned int i = 0; i < nTerrains; i++) {
        lodLevels[i]->addChild(terrains[i]);
        //cout << "  added Child to lodLevel " << lodLevels[i]->getName() << " " << i << endl;
    }
}

void VRWorldGenerator::setTerrainSize( Vec2d in ) { terrainSize = in; }

void VRWorldGenerator::setupLODTerrain(string pathMap, string pathPaint, float scale, bool cache, bool isLit, Color4f mixColor, float mixAmount ) {
#ifndef WITHOUT_GDAL
    cout << "VRWorldGenerator::setupLODTerrain" << endl;
    auto tex = loadGeoRasterData(pathMap, false);
    Vec3i texSizeN = tex->getSize();
    //cout << " texSizeN: " << texSizeN << endl;

    for (auto tt : terrains) tt->destroy();
    terrains.clear();
    ///TODO: angular resolution human eye: 1 arcminute, approximately 0.02° or 0.0003 radians,[1] which corresponds to 0.3 m at a 1 km distance., https://en.wikipedia.org/wiki/Naked_eye

    auto genPath = [&](string filepath, string in, string type){
        string typ = "";
        string res = "";
        for (int i = 0; i < 4; i++) typ = filepath.at(filepath.length()-1-i) + typ;
        if (typ != type) return in+"newFile"+type;
        for (unsigned int i = 0; i < filepath.length()-4; i++) res += filepath.at(i);
        return res + in + type;
    };

    VRTexturePtr tex1;
    VRTexturePtr tex2;
    double geoTransform[6];
    vector<double> gTr;

    string pathMap1 = genPath(pathMap, "_1", ".hgt");
    string pathMap2 = genPath(pathMap, "_2", ".hgt");

    string pathPaint1 = genPath(pathPaint, "_1", ".png");
    string pathPaint2 = genPath(pathPaint, "_2", ".png");

    //cout << " newPathes: " << pathMap1 << endl;
    //cout << " newPathes: " << pathMap2 << endl;
    if ( FILESYSTEM::exist(pathMap1) && cache ) {
        tex1 = loadGeoRasterData(pathMap1, false);
    } else {
        cout << "VRWorldGenerator::setupLODTerrain creating new downsized texture lvl1 at " << pathMap1 << endl;
        tex1 = loadGeoRasterData(pathMap, false);
        tex1->downsize();
        string params[3];
        gTr = getGeoTransform(pathMap);
        for (int i = 0; i < 6; i++) geoTransform[i] = gTr[i];
        geoTransform[1] *= 2;
        writeGeoRasterData(pathMap1, tex1, geoTransform, params);
    }
    if ( FILESYSTEM::exist(pathMap2) && cache ) {
        tex2 = loadGeoRasterData(pathMap2, false);
    } else {
        cout << "VRWorldGenerator::setupLODTerrain creating new downsized texture lvl2 at " << pathMap2 << endl;
        tex2 = loadGeoRasterData(pathMap, false);
        tex2->downsize();
        tex2->downsize();
        string params2[3];
        gTr = getGeoTransform(pathMap);
        for (int i = 0; i < 6; i++) geoTransform[i] = gTr[i];
        geoTransform[1] *= 4;
        writeGeoRasterData(pathMap2, tex2, geoTransform, params2);
    }
    if ( !FILESYSTEM::exist(pathPaint1) || !cache ) {
        cout << "VRWorldGenerator::setupLODTerrain creating new downsized sat texture lvl1 at " << pathPaint1 << endl;
        VRTexturePtr dsSatImg1 = VRTexture::create();
        dsSatImg1->read(pathPaint);
        dsSatImg1->downsize();
        dsSatImg1->write(pathPaint1);
    }
    if ( !FILESYSTEM::exist(pathPaint2) || !cache ) {
        cout << "VRWorldGenerator::setupLODTerrain creating new downsized sat texture lvl2 at " << pathPaint2 << endl;
        VRTexturePtr dsSatImg2 = VRTexture::create();
        dsSatImg2->read(pathPaint);
        dsSatImg2->downsize();
        dsSatImg2->downsize();
        dsSatImg2->write(pathPaint2);
    }

    auto addTerrain = [&](double fac, int a) {
        auto terrain = VRTerrain::create("terrain"+toString(fac));
        fac*=0.8;
        terrain->setParameters (terrainSize, 2/fac, 1);
        VRTexturePtr texSc = tex;
        string satImg = pathPaint;
        if (a == 1) { texSc = tex1; satImg = pathPaint1; }
        if (a == 2) { texSc = tex2; satImg = pathPaint2; }
        if ( !FILESYSTEM::exist(pathPaint2) ) satImg = pathPaint;
    //if (mixAmount > 0) texSc->mixColor(mixColor, mixAmount);
        terrain->paintHeights( satImg, mixColor, mixAmount );
        //terrain->paintHeights( satImg, Color4f(1,0,1,1), 0.5 );
        terrain->setMap( texSc, 3 );
        terrain->setWorld( ptr() );
        terrain->setLODFactor(fac);
        terrain->setLit(isLit);
        terrains.push_back(terrain);
    };

    addTerrain(1.0, 0);
    addTerrain(0.5, 1);
    addTerrain(0.05, 2);
#endif
}

vector<VRTerrainPtr> VRWorldGenerator::getTerrains(){
    return terrains;
}

void VRWorldGenerator::setupLOD(int layers){
    lodLevels.clear();
    lodFactors.clear();
    for (auto tt:terrains) tt->destroy();
    terrains.clear();
    if (lod) lod->destroy();
    lod = VRLod::create("wgenLod");
    auto addLod = [&](string name, double d, double fac) {
        auto obj = VRObject::create(name);
        lodLevels.push_back(obj);
        lodFactors.push_back(fac);
        lod->addChild(obj);
        lod->addDistance(d);
    };
    ///TODO: make layer distance dependent of planet scale and radius
    if ( layers == 1 ) { addLod( "wgenlvl0", 10000000.0, 1.0 ); }
    if ( layers > 1 ) {
        addLod( "wgenlvl0", 5000.0, 1.0 );
        addLod( "wgenlvl1", 15000.0, 0.5 );
        addLod( "wgenlvl2", 300000.0, 0.05);
    }
    auto anchor = VRObject::create("wgenAnchor");
    lodLevels.push_back(anchor);
    lodFactors.push_back(-1);
    lod->addChild(anchor);
    addChild(lod);
}

void VRWorldGenerator::setLODTerrainParameters(float heightScale) {
    for (auto each : terrains){
        each->setHeightScale(heightScale);
    }
}

void VRWorldGenerator::processOSMMap(double subN, double subE, double subSize) {
    if (!ontology) { cout << "Warning: no ontology found! ..skipping processOSMMap" << endl; return; }

    struct Node {
        OSMNodePtr n;
        VREntityPtr e;
        Vec3d p;
    };

    map<string, Node> graphNodes;
    map<string, VRRoadPtr> RoadEntities;

    auto startswith = [](const string& a, const string& b) -> bool {
        if (a.size() < b.size()) return false;
        return a.compare(0, b.length(), b) == 0;
    };

    auto wayToPolygon = [&](OSMWayPtr& way) {
        auto poly = VRPolygon::create();
        for (auto ps : way->polygon.get()) {
            auto p = planet->fromLatLongPosition(ps[1], ps[0], true);
            poly->addPoint( Vec2d(p[0], p[2]) );
        }
        if (!poly->isCCW()) poly->reverseOrder();
        return poly;
    };

    auto wayToPath = [&](OSMWayPtr& way, int N) -> PathPtr {
        auto path = Path::create();
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
            if (terrains.size() == 1) {
                p = terrains[0]->elevatePoint(p,p[1]);
                terrains[0]->projectTangent(d, p);
            } else d.normalize();
            path->addPoint( Pose( p, d ) );
        };

        if (pos.size() < 2) return 0;
        if (pos.size() == 2) {
            Vec3d d = pos[1] - pos[0];
            addPnt(pos[0], d);
            addPnt(pos[1], d);
        }

        for (unsigned int i=1; i<pos.size()-1; i++) {
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
        auto path = Path::create();
        vector<Vec3d> pos;
        for (auto nID : way->nodes) {
            auto n = osmMap->getNode(nID);
            Vec3d p = planet->fromLatLongPosition(n->lat, n->lon, true);
            pos.push_back( p );
        }

        auto addPnt = [&](Vec3d p, Vec3d d) {
            if (terrains.size() == 1) {
                p = terrains[0]->elevatePoint(p);
                terrains[0]->projectTangent(d, p);
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

        for (unsigned int i=1; i<pos.size()-1; i++) {
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

    auto addTunnel = [&](OSMWayPtr& way, VRRoadPtr road) {
        if (way->hasTag("layer")) return;
        roads->addTunnel(road);
    };

    auto addBridge = [&](OSMWayPtr& way, VRRoadPtr road) {
        ;
    };

    std::function<void(OSMWayPtr&, string, float, bool)> addRoad = [&](OSMWayPtr& way, string tag, float width, bool pedestrian) -> void {
        if (way->nodes.size() < 2) return;

        int Nsegments = 0.5 + way->nodes.size()/20.0;
        if (Nsegments > 1) { // too long, split it up!
            for (auto w : osmMap->splitWay(way, Nsegments)) {
                addRoad(w, tag, width, pedestrian);
            }
            return;
        }

        string name = "road";
        if (way->hasTag("name")) name = way->tags["name"];

        vector<VREntityPtr> nodes;
        vector<Vec3d> norms;

        auto getInt = [&](string tag, int def) { return way->hasTag(tag) ? toInt( way->tags[tag] ) : def; };
        //auto getFloat = [&](string tag, float def) { return way->hasTag(tag) ? toFloat( way->tags[tag] ) : def; };
        auto getString = [&](string tag, string def) { return way->hasTag(tag) ? way->tags[tag] : def; };

        auto addPathData = [&](VREntityPtr node, Vec3d pos, Vec3d norm) {
            if (terrains.size() == 1) terrains[0]->projectTangent(norm, pos);
            else norm.normalize();
            nodes.push_back(node);
            norms.push_back(norm);
        };

        for (unsigned int i=1; i<way->nodes.size(); i++) {
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
        if (!way->hasTag("lanes:forward") && !way->hasTag("lanes:backward") && !way->hasTag("lanes") && !way->hasTag("oneway")){
            if (tag == "residential") { NlanesRight = 1; NlanesLeft = 1; }
            if (tag == "service") { NlanesRight = 1; NlanesLeft = 1; }
            if (tag == "living_street") { NlanesRight = 1; NlanesLeft = 1; }
        }
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

        // add tag for maximum speed to lanes
        //TODO: might need to parse in the future
        // ^([0-9][\.0-9]+?)(?:[ ]?(?:km/h|kmh|kph|mph|knots))?$
        vector<string> maxSpeeds;
        if ( way->hasTag("maxspeed:lanes") ) {
            auto data = splitString( way->tags["maxspeed:lanes"], '|' );
            for (auto d : data) maxSpeeds.push_back(d);
        } else if ( way->hasTag("maxspeed") ) {
            maxSpeeds = vector<string>( NlanesRight+NlanesLeft, way->tags["maxspeed"] );
        } else maxSpeeds = vector<string>( NlanesRight+NlanesLeft, "50.0");

        // create road and lane entities
        auto road = roads->addLongRoad(name, tag, nodes, norms, 0);
        if (hasPLaneR) road->addParkingLane(1, widths[NlanesRight], getInt("parking:lane:right:capacity", 0), getString("parking:lane:right", ""));
        for (int l=0; l < NlanesRight; l++) { auto lane = road->addLane(1, widths[NlanesRight-1-l], pedestrian); lane->set("maxspeed", maxSpeeds[NlanesRight-1-l]); }
        for (int l=0; l < NlanesLeft; l++) { auto lane = road->addLane(-1, widths[NlanesRight+hasPLaneR+l], pedestrian); lane->set("maxspeed", maxSpeeds[NlanesRight+l]); }
        if (hasPLaneL) road->addParkingLane(-1, widths[NlanesRight+NlanesLeft+hasPLaneR], getInt("parking:lane:left:capacity", 0), getString("parking:lane:left", ""));
        RoadEntities[way->id] = road;

        // check for tunnels and bridges
        if (way->hasTag("tunnel")) addTunnel(way, road);
        if (way->hasTag("bridge")) addBridge(way, road);
    };

    auto addBuilding = [&](OSMWayPtr& way) {
        int lvls = 4;
        string housenumber = "";
        string street = "";
        if (way->hasTag("building:levels")) lvls = toInt( way->tags["building:levels"] );
        if (way->hasTag("addr:housenumber")) housenumber = way->tags["addr:housenumber"];
        if (way->hasTag("addr:street")) street = way->tags["addr:street"];
        string bType = "residential";
        if (rand()%2) bType = "shopping";
        district->addBuilding( wayToPolygon(way), lvls, housenumber, street, bType );
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
            terrains[0]->addEmbankment(rel->id, p1, p2, p3, p4);
        }
    }

    for (auto wayItr : osmMap->getWays()) { // use way->id to filter for debugging!
        auto& way = wayItr.second;
        if (!wayInSubarea(way)) continue;

        vector<Vec3d> pnts;
        for (auto pID : way->nodes) {
            if (graphNodes.count(pID)) continue;
            Node n;
            n.n = osmMap->getNode(pID);
            if (!n.n) continue;
            n.p = planet->fromLatLongPosition(n.n->lat, n.n->lon, true);
            pnts.push_back(n.p);
            graphNodes[pID] = n;
        }
        bool wayAdded = false;

        if (userCbPtr) {
            OsmEntity e;
            e.pnts = pnts;
            e.tags = way->tags;
            if ( !(*userCbPtr)(e) ) continue;
        }

        auto getHeight = [&](float d) {
            return way->hasTag("height") ? toFloat( way->tags["height"] ) : d;
        };

        for (auto tag : way->tags) {
            if (tag.first == "highway") {
                if (tag.second == "footway" || tag.second == "pedestrian") {
                    bool addedFootway = false;
                    /*for (auto tag : way->tags) {
                        if (!addedFootway && ( (tag.first == "crossing" && tag.second == "zebra") || (tag.first == "footway" && tag.second == "crossing")) ) {
                            addRoad(way, "crossing", 3, true);
                            addedFootway = true;
                            continue;
                        }
                    }*/
                    if (!addedFootway) { addRoad(way, "footway", 1, true); wayAdded = true; }
                    continue;
                }
                addRoad(way, tag.second, 4, false); // default road
                wayAdded = true;
                continue;
            }

            if (tag.first == "building") {
                addBuilding(way);
                wayAdded = true;
                continue;
            }

            if (tag.first == "barrier") {
                if (tag.second == "guard_rail") {
                    roads->addGuardRail( wayToPath(way, 8), getHeight(0.5) );
                    wayAdded = true;
                }
                if (tag.second == "kerb") {
                    roads->addKirb( wayToPolygon(way), getHeight(0.2) );
                    wayAdded = true;
                }
                if (tag.second == "fence") {
                    roads->addFence( wayToPath(way, 8), getHeight(1.8) );
                    wayAdded = true;
                }
                continue;
            }

            if (tag.first == "natural") {
                //if (tag.second == "woods") nature->addWoods( wayToPolygon(way), 1);
                if (tag.second == "scrub") { nature->addScrub( wayToPolygon(way), 1); wayAdded = true; }
                //if (tag.second == "grassland") nature->addGrassPatch( wayToPolygon(way), 0, 1, 0);
                continue;
            }

            if (tag.first == "landuse") { // TODO
                if (tag.second == "industrial") {
                    auto patch = VRGeometry::create("patch");
                    patch->hide("SHADOW");
                    auto poly = wayToPolygon(way);
                    if (poly->size() == 0) continue;
                    for (auto p : poly->gridSplit(5)) {
                        if (terrains.size()) terrains[0]->elevatePolygon(p, 0.03, false);
#ifndef WITHOUT_GLU_TESS
                        Triangulator tri;
                        tri.add(*p);
                        patch->merge( tri.compute() );
#endif
                    }
                    patch->setMaterial(roads->getMaterial());
                    patch->setPositionalTexCoords(1.0, 0, Vec3i(0,2,1));
                    addChild(patch);
                    wayAdded = true;
                    continue;
                }
            }
        }
        if (!wayAdded) {
            //TODO: might want to check if ways are closed loops or just undefined ways/roads
            auto areaEnt = ontology->addEntity("miscArea", "MiscArea");
            for (auto tag : way->tags) {
                string newTag = tag.first+":"+tag.second;
                areaEnt->add("tags", newTag);
            }

            auto patch = VRGeometry::create("miscArea");
            patch->hide("SHADOW");
            auto poly = wayToPolygon(way);
            if (poly->size() == 0) continue;
            for (auto p : poly->gridSplit(5)) {
                if (terrains.size() == 1) terrains[0]->elevatePolygon(p, 0.03, false);
#ifndef WITHOUT_GLU_TESS
                Triangulator tri;
                tri.add(*p);
                patch->merge( tri.compute() );
#endif
            }
            patch->updateNormals();
            VRGeoData nData(patch);
            for (int i=0; i<nData.sizeNormals(); i++) {
                auto n = nData.getNormal(i);
                if (n[1] < 0) nData.setNorm(i,-n);
            }
            patch->setMaterial(roads->getMaterial());
            patch->setPositionalTexCoords(1.0, 0, Vec3i(0,2,1));
            addChild(patch);
            miscAreaByEnt[areaEnt] = patch;
            wayAdded = true;
            continue;
        }
    }

    for (auto nodeItr : osmMap->getNodes()) {

        auto& node = nodeItr.second;
        if (!nodeInSubarea(node)) continue;
        Vec3d pos = planet->fromLatLongPosition(node->lat, node->lon, true);

        if (userCbPtr) {
            OsmEntity e;
            e.pnts = {pos};
            e.tags = node->tags;
            if ( !(*userCbPtr)(e) ) continue;
        }

        Vec3d dir = getDir(node);
        bool hasDir = node->tags.count("direction");
        if (terrains.size() == 1) pos = terrains[0]->elevatePoint(pos);
        //bool addToOnto = false;
        bool added = false;
        for (auto tag : node->tags) {
            if (added) continue;
            if (tag.first == "natural") {
                if (tag.second == "tree") nature->createRandomTree(pos);
                continue;
            }

            //if (tag.first == "traffic_sign:training_ground") {
            if (startswith(tag.first, "traffic_sign") && !startswith(tag.first, "traffic_signals")) {
                auto signEnt = ontology->addEntity("sign", "Sign");
                added = true; //making sure not to add twice
                /*if ((tag.second == "yes" || tag.second == "custom") && node->tags.count("name")) {
                    signEnt->set("type", node->tags["name"]);
                } else signEnt->set("type", tag.second);*/
                bool tmpc = false; //check if custom sign with name
                bool revDir = false; //check if sign facing other way
                bool nDir = false; //check if sign facing other way
                Vec3d tmp = dir;
                for ( auto tagN : node->tags ) {
                    if ( tagN.first == "traffic_sign" && (tagN.second == "yes" || tagN.second == "custom" || tagN.second == "*") && node->tags.count("name") ) { signEnt->set("type", node->tags["name"]); tmpc = true; }
                    if ( tagN.first == "traffic_sign" && startswith(tagN.second,"CN:") ) {
                        //cout << "-----------OSMsign" << endl;
                        signEnt->set("type", "OSMSign");
                        signEnt->set("info",tagN.second);
                        if (tagN.second == "CN:Prohibitory:5") nDir = true;
                        tmpc = true;
                    }
                    //if ( startswith(tagN.first,"traffic_sign") && (tagN.second == "yes" || tagN.second == "custom") && node->tags.count("name") && !tmpc ) { signEnt->set("type", node->tags["name"]); tmpc = true; }
                    if ( tagN.first == "traffic_sign:backward" ) { tmp = -dir; revDir = true; }
                }
                if ( !tmpc ) signEnt->set("type", tag.second);
                if ( nDir ) revDir = false;
                signEnt->setVec3("position", pos, "Position");
                signEnt->setVec3("direction", dir, "Direction");
                for (auto way : node->ways) {
                    if (!RoadEntities.count(way)) continue;
                    auto road = RoadEntities[node->ways[0]];
                    if (!road) continue;
                    auto roadEnt = road->getEntity();
                    for (auto laneEnt : roadEnt->getAllEntities("lanes")) {
                        auto laneDir = laneEnt->getValue("direction", 1);
                        //Vec3d laneTangent = road->getRightEdge(pos)->dir() * laneDir;
                        if (( !hasDir && ( ( revDir && laneDir<0 ) || ( !revDir && laneDir>0 ) ) ) ) { //tmp.dot(laneTangent) < -0.5 ||
                            laneEnt->add("signs",signEnt->getName());
                            signEnt->add("lanes",laneEnt->getName());
                        }
                    }
                }
            }

            if (tag.first == "information"){
                continue;
                auto signEnt = ontology->addEntity("sign", "Sign");
                bool tmpc = false;
                if ( node->tags.count("name") ) { signEnt->set("type", node->tags["name"]); tmpc = true; }
                if ( !tmpc ) signEnt->set("type", tag.second);

                signEnt->setVec3("position", pos, "Position");
                signEnt->setVec3("direction", dir, "Direction");
            }

            if (startswith(tag.first, "traffic_signals")) {
                added = true;
                //cout << " VRWorldGenerator::processOSMMap tr_signal " << endl;
                for (auto way : node->ways) {
                    if (!RoadEntities.count(way)) continue;
                    auto road = RoadEntities[node->ways[0]];
                    road->addTrafficLight(pos);
                    //cout << "    VRWorldGenerator::processOSMMap " <<toString(road->getID()) << endl;
                }
            }

            if (tag.first == "surveillance:type") {
                if (tag.second == "camera") {
                    auto cam = assets->copy("Camera", Pose::create(pos, dir), false);
                    lodTree->addObject(cam, cam->getWorldPosition(), 3, false);
                    cam->setDir(dir);
                    Pose p(pos, dir);
#ifndef WITHOUT_BULLET
                    collisionShape->addQuad(0.1, 2, p, cam->getID());
#endif
                }
            }

            if (tag.first == "highway") {
                added = true;
                if (tag.second == "street_lamp") {
                    auto lamp = assets->copy("Streetlamp", Pose::create(pos, -dir), false);
                    if (lamp) {
                        lodTree->addObject(lamp, lamp->getWorldPosition(), 3, false);
                        lamp->setDir(-dir);
                        Pose p(pos, -dir);
#ifndef WITHOUT_BULLET
                        collisionShape->addQuad(0.1, 2, p, lamp->getID());
#endif
                    }
                }
                if (tag.second == "traffic_signals") {
                   //cout << " VRWorldGenerator::processOSMMap tr_signal " << endl;
                    for (auto way : node->ways) {
                        if (!RoadEntities.count(way)) continue;
                        auto road = RoadEntities[node->ways[0]];
                        road->addTrafficLight(pos);
                        //cout << "    VRWorldGenerator::processOSMMap " <<toString(road->getID()) << endl;
                    }
                }
            }
        }
        if (!added) {
            auto nodeEnt = ontology->addEntity("misc", "Misc");
            nodeEnt->setVec3("position", pos, "Position");
            nodeEnt->setVec3("direction", dir, "Direction");
            for (auto tag : node->tags) {
                string newTag = tag.first+":"+tag.second;
                nodeEnt->add("tags", newTag);
            }
        }
    }
}

void VRWorldGenerator::processGMLfromOSM(){
    if (!gmlMap) return;
    cout << "VRWorldGenerator::processGMLfromOSM" << endl;

    auto GMLGeometries = VRObject::create("GMLGeometries");
    auto mat = VRMaterial::create("GMLMaterial");
    mat->setPointSize(15);
    addChild(GMLGeometries);
    int counter = 0;

    for (auto wayItr : gmlMap->getWays()) { // use way->id to filter for debugging!
        auto& way = wayItr.second;

        auto poly = VRPolygon::create();

        int nID = 0;
        for (auto pID : way->nodes) {
            auto node = gmlMap->getNode(pID);
            if (!node) continue;
            auto p = planet->fromLatLongElevationPose(node->lat, node->lon, node->elevation, true, true)->pos();
            poly->addPoint( p );
            nID++;
        }
        if (poly->size() == 0) continue;
        poly->close();
        if (!poly->isCCW()) poly->reverseOrder();

        auto patch = VRGeometry::create("polygonGML");
        patch->hide("SHADOW");
#ifndef WITHOUT_GLU_TESS
        Triangulator tri;
        tri.add(*poly);
        patch->merge( tri.compute() );
#endif
        patch->updateNormals();
        GMLGeometries->addChild(patch);
        counter++;

        for (auto tag : way->tags) {
            //cout << tag.first << " " << tag.second << endl;
        }
    }
    cout << "  generated: " << counter << " geometries"<< endl;
    //addChild();
}

void VRWorldGenerator::setUserCallback(VRUserGenCbPtr cb) { userCbPtr = cb; }

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
    for (auto terrain:terrains) terrain->clear();
    nature->clear();
    miscAreaByEnt.clear();
}

string VRWorldGenerator::getStats() {
    string res;
    res += "world generator stats:\n";
    res += " OSM map: " + toString(osmMap->getMemoryConsumption()) + " mb\n";
    res += " Road network: " + toString(roads->getMemoryConsumption()) + " mb\n";
    return res;
}




