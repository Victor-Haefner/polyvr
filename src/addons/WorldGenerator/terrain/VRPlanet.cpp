#include "VRPlanet.h"
#include "../VRWorldGenerator.h"
#include "VRTerrain.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRLod.h"
#include "core/tools/VRAnalyticGeometry.h"
#include "core/math/pose.h"
#include "core/utils/toString.h"

#define GLSL(shader) #shader

const double pi = 3.14159265359;

using namespace OSG;

template<> string typeName(const VRPlanet& t) { return "Planet"; }

VRPlanet::VRPlanet(string name) : VRTransform(name) {}
VRPlanet::~VRPlanet() {}

VRPlanetPtr VRPlanet::create(string name) {
    auto p = VRPlanetPtr( new VRPlanet(name) );
    p->rebuild();
    return p;
}

VRPlanetPtr VRPlanet::ptr() { return static_pointer_cast<VRPlanet>( shared_from_this() ); }

double VRPlanet::toRad(double deg) { return pi*deg/180; }
double VRPlanet::toDeg(double rad) { return 180*rad/pi; }

void VRPlanet::localize(double north, double east) {
    originCoords = Vec2d(north, east);
    auto p = fromLatLongPose(north, east);
    auto newUp = p->pos();
    newUp.normalize();
    auto localOrigin = p->pos();
    p->invert();
    origin->setPose(p);
    auto dirOrigin = origin->getDir();
    dirOrigin.normalize();
    auto upOrigin = origin->getUp();
    upOrigin.normalize();

    //auto ma2 = fromLatLongPose(north, east)->multRight(fromLatLongPose(p));

    lod->hide(); // TODO: work around due to problems with intersection action!!

    auto metaWorld = VRAnalyticGeometry::create("metaWorld");
    metaWorld->setLabelParams(0.02, true, true, Color4f(0.5,0.1,0,1), Color4f(1,1,0.5,1));
    auto metaSec1 = VRAnalyticGeometry::create("metaSec1");
    metaSec1->setLabelParams(0.02, true, true, Color4f(0.5,0.1,0,1), Color4f(1,1,0.5,1));

    int nSec = 0;
    VRWorldGeneratorPtr sec1;
    for (auto s : sectors) {
        if (nSec==0) sec1 = s.second;
        nSec++;
    }
    auto nnn1 = sec1->getPlanetCoords()[0];
    auto eee1 = sec1->getPlanetCoords()[1];
    auto pvecMID = fromLatLongPose(nnn1+0.5*sectorSize, eee1+0.5*sectorSize);

    for (auto s : sectors) {
        auto sector = s.second;
        addChild(sector);
        sector->setIdentity();

        Vec2d plI = sector->getPlanetCoords() + Vec2d(1,1)*sectorSize*0.5;
        auto pSector = fromLatLongPose(plI[0], plI[1]);

        auto newP = p->multRight(pSector);
        sector->setPose(newP);
        auto newPinv = newP;
        newPinv->invert();

        Vec2d size = sector->getTerrain()->getSize();
        Vec2i gridN = Vec2i(round(size[0]*1.0/grid-0.5), round(size[1]*1.0/grid-0.5));
        if (gridN[0] < 1) gridN[0] = 1;
        if (gridN[1] < 1) gridN[1] = 1;
        Vec2d gridS = size;
        gridS[0] /= gridN[0];
        gridS[1] /= gridN[1];

        vector<vector<Vec3d>> completeMesh;

        for (int i =0; i < gridN[0]; i++) {
            for (int j =0; j < gridN[1]; j++) {
                //Vertex conversion from global to local patch coordinates
                auto poseVertexGlobal = fromLatLongPose(north+i*sectorSize/gridN[0], east+j*sectorSize/gridN[1]);
                auto poseVertexOrigin = p->multRight(poseVertexGlobal);
                auto poseVertexLocalInPatch = newPinv->multRight(poseVec1);
                auto posVertexLocalInPatch = poseVertexLocalInPatch->pos();
                row.push_back(posVertexLocalInPatch);
            }
            completeMesh.push_back(row);
        }

        //Vertex conversion from global to local patch coordinates
        auto poseVertexGlobal = fromLatLongPose(nn1, ee1);
        auto poseVertexOrigin = p->multRight(poseVertexGlobal);
        auto poseVertexLocalInPatch = newPinv->multRight(poseVec1);
        auto posVertexLocalInPatch = poseVertexLocalInPatch->pos();

        auto pvec1 = fromLatLongPose(nn1, ee1);
        auto pvec2 = fromLatLongPose(nn1+sectorSize, ee1);
        auto pvec3 = fromLatLongPose(nn1+sectorSize, ee1+sectorSize);
        auto pvec4 = fromLatLongPose(nn1, ee1+sectorSize);
        auto pvec5 = fromLatLongPose(nn1+0.5*sectorSize, ee1+0.5*sectorSize);

        auto poseVec1 = p->multRight(pvec1);
        auto poseVec2 = p->multRight(pvec2);
        auto poseVec3 = p->multRight(pvec3);
        auto poseVec4 = p->multRight(pvec4);
        auto poseVec5 = p->multRight(pvec5);

        auto vec1l = poseVec1->pos();
        auto vec2l = poseVec2->pos();
        auto vec3l = poseVec3->pos();
        auto vec4l = poseVec4->pos();
        auto vec5l = poseVec5->pos();
        metaSec1->setVector(IDID, Vec3d(vec1l), Vec3d(0,1,0)*1000, Color3f(1,0,0.5), "", false); IDID++;
        metaSec1->setVector(IDID, Vec3d(vec2l), Vec3d(0,1,0)*1000, Color3f(1,0,0.5), "", false); IDID++;
        metaSec1->setVector(IDID, Vec3d(vec3l), Vec3d(0,1,0)*1000, Color3f(1,0,0.5), "", false); IDID++;
        metaSec1->setVector(IDID, Vec3d(vec4l), Vec3d(0,1,0)*1000, Color3f(1,0,0.5), "", false); IDID++;
        metaSec1->setVector(IDID, Vec3d(vec5l), Vec3d(0,1,0)*1000, Color3f(1,0,0.5), "", false); IDID++;

        auto ppVec1 = newPinv;
        auto ppVec2 = newPinv;
        auto ppVec3 = newPinv;
        auto ppVec4 = newPinv;
        auto ppVec5 = newPinv;

        auto posevec1loc = ppVec1->multRight(poseVec1);
        auto posevec2loc = ppVec2->multRight(poseVec2);
        auto posevec3loc = ppVec3->multRight(poseVec3);
        auto posevec4loc = ppVec4->multRight(poseVec4);
        auto posevec5loc = ppVec5->multRight(poseVec5);

        auto vec1loc = posevec1loc->pos();
        auto vec2loc = posevec2loc->pos();
        auto vec3loc = posevec3loc->pos();
        auto vec4loc = posevec4loc->pos();
        auto vec5loc = posevec5loc->pos();
        metaGeoSector->setVector(IDID, Vec3d(vec1loc), Vec3d(0,1,1)*1000, Color3f(0,0,1), "", false); IDID++;
        metaGeoSector->setVector(IDID, Vec3d(vec2loc), Vec3d(0,1,1)*1000, Color3f(0,0,1), "", false); IDID++;
        metaGeoSector->setVector(IDID, Vec3d(vec3loc), Vec3d(0,1,1)*1000, Color3f(0,0,1), "", false); IDID++;
        metaGeoSector->setVector(IDID, Vec3d(vec4loc), Vec3d(0,1,1)*1000, Color3f(0,0,1), "", false); IDID++;
        metaGeoSector->setVector(IDID, Vec3d(vec5loc), Vec3d(0,1,1)*1000, Color3f(0,0,1), "", false); IDID++;

        auto vec1g = pvec1->pos();
        auto vec2g = pvec2->pos();
        auto vec3g = pvec3->pos();
        auto vec4g = pvec4->pos();
        auto vec5g = pvec5->pos();
        metaWorld->setVector(IDID, Vec3d(vec1g), Vec3d(0,1,0)*1000, Color3f(0,1,0.5), "", false); IDID++;
        metaWorld->setVector(IDID, Vec3d(vec2g), Vec3d(0,1,0)*1000, Color3f(0,1,0.5), "", false); IDID++;
        metaWorld->setVector(IDID, Vec3d(vec3g), Vec3d(0,1,0)*1000, Color3f(0,1,0.5), "", false); IDID++;
        metaWorld->setVector(IDID, Vec3d(vec4g), Vec3d(0,1,0)*1000, Color3f(0,1,0.5), "", false); IDID++;
        metaWorld->setVector(IDID, Vec3d(vec5g), Vec3d(0,1,0)*1000, Color3f(0,1,0.5), "c", false); IDID++;

        sector->addChild(metaGeoSector);
        sector->getTerrain()->setMeshTer(completeMesh);
        cout << " SecNormal: " << newP->pos() << " " << newP->dir()<< " " << newP->up()<< endl;
        cout << " plI: " << plI <<  endl;
        cout << " newP: " << newP <<  endl;
        //cout << " SecNorth: " << fromLatLongNorth(plI[], plI[1]) << endl;
        //cout << "VRPlanet::localize p " << p << " pSector: " << pSector << " localOrigin: " << localOrigin << endl;

        /*
        Vec2d size = sector->getTerrain()->getSize();
        Vec2d p = sector->getPlanetCoords() + Vec2d(1,1)*sectorSize*0.5; // sector mid point
        float X = p[0]-east;
        float Y = north - p[1];
        cout << "VRPlanet::localize p " << p << " P " << Vec2f(north, east) << " XY " << Vec2d(X, Y) << endl;
        //cout << "VRPlanet::localize " << Y << " " << p[0]*sectorSize << " " << north << endl;
        sector->translate(Vec3d(X*size[0]/sectorSize, 0, Y*size[1]/sectorSize));*/
    }

    sec1->addChild(metaSec1);
    origin->addChild(metaWorld);

    /*auto s = getSector(north, east);
    if (s) {
        s->setIdentity();
        addChild(s);
    } else cout << "Warning: VRPlanet::localize, no sector found at location " << Vec2d(north, east) << " !\n";*/
}

Vec2i VRPlanet::toSID(double north, double east) {
    return Vec2i( north/sectorSize+1e-9, east/sectorSize+1e-9 );
}

Vec3d VRPlanet::fromLatLongNormal(double north, double east, bool local) {
    if (local) {
        north -= originCoords[0] - 90;
        east  -= originCoords[1];
    }

    north = -north+90;
	double sT = sin(toRad(north));
	double sP = sin(toRad(east));
	double cT = cos(toRad(north));
	double cP = cos(toRad(east));
	return Vec3d(sT*cP, cT, -sT*sP);
}

Vec3d VRPlanet::fromLatLongPosition(double north, double east, bool local) {
    Vec3d pos;
    if (local) {
        auto s = fromLatLongSize(originCoords[0]-0.5, originCoords[1]-0.5, originCoords[0]+0.5, originCoords[1]+0.5);
        pos = Vec3d( (east-originCoords[1])*s[0], 0, -(north-originCoords[0])*s[1]);

        /*auto p = fromLatLongPose(originCoords[0], originCoords[1], 0); // DEPRECATED, numerically unstable!
        p->invert();
        p->asMatrix().mult(pos, pos);*/
    } else pos = fromLatLongNormal(north, east, 0) * radius;
    return Vec3d( pos );
}

Vec3d VRPlanet::fromLatLongEast(double north, double east, bool local) { return fromLatLongNormal(0, east+90, local); }
Vec3d VRPlanet::fromLatLongNorth(double north, double east, bool local) { return fromLatLongNormal(north+90, east, local); }

PosePtr VRPlanet::fromLatLongPose(double north, double east, bool local) {
    Vec3d f = fromLatLongPosition(north, east, local);
    Vec3d d = fromLatLongNorth(north, east, local);
    Vec3d u = fromLatLongNormal(north, east, local);
    return Pose::create(f,d,u);
}

Vec2d VRPlanet::fromLatLongSize(double north1, double east1, double north2, double east2) {
    auto n = (north1+north2)*0.5;
    auto e = (east1+east2)*0.5;
    Vec3d p1 = fromLatLongPosition(north1, east1);
    Vec3d p2 = fromLatLongPosition(north2, east2);
    //addPin("P1", north1, east1);
    //addPin("P2", north2, east2);
    Vec3d d = p2-p1;
    auto dirEast = fromLatLongEast(n, e);
    auto dirNorth = fromLatLongNorth(n, e);
    double u = d.dot( dirEast );
    double v = d.dot( dirNorth );

    /*d = fromLatLongPosition(north2, east2) - fromLatLongPosition(north2, east1);
    metaGeo->setVector(101, Vec3d(p1), Vec3d(d), Color3f(1,1,0), "D");
    metaGeo->setVector(102, Vec3d(p1), Vec3d(dirEast*u), Color3f(0,1,1), "E");
    metaGeo->setVector(103, Vec3d(p1), Vec3d(dirNorth*v), Color3f(0,1,1), "S");
    */
    return Vec2d(abs(u),abs(v));
}

Vec2d VRPlanet::fromPosLatLong(Pnt3d p, bool local) { // TODO: increase resolution by enhancing getWorldMatrix
    Pnt3d p1 = p;
    if (local) {
        auto m = origin->getWorldMatrix();
        m.invert();
        m.mult(p,p1);
    }

    Vec3d n = Vec3d(p1);
    n.normalize();
    double north = -toDeg(acos(n[1]))+90;
    double east = toDeg(atan2(-n[2], n[0]));

    if (local) {
        // increase precision by 2D planar approximation
        Vec2d s = fromLatLongSize(north-0.5, east-0.5, north+0.5, east+0.5);
        Vec3d p2 = fromLatLongPosition(north, east, local);
        int pr = cout.precision();
        cout.precision(17);
        cout << "p1: " << p << " p2: " << p2 << endl;
        Vec3d d = Vec3d(p)-p2;
        cout << std::setprecision (15) << Vec2d(north, east);
        north += -d[2]*1.0/s[1];
        east  += d[0]*1.0/s[0];
        cout << std::setprecision (15) << " -> " << Vec2d(north, east) << "  d: " << d << " s: " << s << endl;
        cout.precision(pr);
    }

    cout << "VRPlanet::fromPosLatLong p:" << p1 << " n:" << n << " acos:" << acos(n[1]) << " atan2: " << atan2(-n[2], n[0]) << endl;
    return Vec2d(north, east);
}

void VRPlanet::rebuild() {
    // sphere material
    if (!sphereMat) {
        sphereMat = VRMaterial::create("planet");
        sphereMat->setVertexShader(surfaceVP, "planet surface VS");
        sphereMat->setFragmentShader(surfaceFP, "planet surface FS");
        setLit(false);
    }

    // spheres
    if (lod) lod->destroy();
    lod = VRLod::create("planetLod");
    auto addLod = [&](int i, double d) {
        auto s = VRGeometry::create(getBaseName()+"Sphere");
        s->setPrimitive("Sphere " + toString(radius)+" "+toString(i));
        s->setMaterial(sphereMat);
        lod->addChild(s);
        lod->addDistance(d);
    };

    if (!origin) {
        origin = VRTransform::create("origin");
        addChild(origin);
    }
    origin->addChild(lod);
    anchor = VRObject::create("lod0");
    lod->addChild( anchor );
    addLod(5,radius*1.1);
    addLod(4,radius*2.0);
    addLod(3,radius*5.0);

    // init meta geo
    if (!metaGeo) {
        metaGeo = VRAnalyticGeometry::create("PlanetMetaData");
        metaGeo->setLabelParams(0.02, true, true, Color4f(0.5,0.1,0,1), Color4f(1,1,0.5,1));
        origin->addChild(metaGeo);
    }
}

void VRPlanet::setParameters( double r, string t, bool l, double s ) {
    radius = r;
    sectorSize = s;
    rebuild();
    setupMaterial(t, l);
}

void VRPlanet::setLayermode( string mode ) {
    if (mode == "full") layermode = 0;
    if (mode == "minimum") layermode = 1;
}

VRWorldGeneratorPtr VRPlanet::addSector( double north, double east ) {
    auto generator = VRWorldGenerator::create(layermode);
    auto sid = toSID(north, east);
    sectors[sid] = generator;
    anchor->addChild(generator);
    generator->setPlanet(ptr(), Vec2d(north, east));
    generator->setPose( fromLatLongPose(north+0.5*sectorSize, east+0.5*sectorSize) );

    Vec2d size = fromLatLongSize(north, east, north+sectorSize, east+sectorSize);
    vector<Vec3d> vecPos;
    auto ppp = fromLatLongPosition(north+0.5*sectorSize, east+0.5*sectorSize);
    vecPos.push_back(fromLatLongPosition(north, east)-ppp);
    vecPos.push_back(fromLatLongPosition(north+sectorSize, east)-ppp);
    vecPos.push_back(fromLatLongPosition(north+sectorSize, east+sectorSize)-ppp);
    vecPos.push_back(fromLatLongPosition(north, east+sectorSize)-ppp);
    //generator->getTerrain()->setEdgepoints( vecPos );

    auto poss = fromLatLongPosition(north,east);

    auto grid = 5000;
    Vec2i gridN = Vec2i(round(size[0]*1.0/grid-0.5), round(size[1]*1.0/grid-0.5));
    if (gridN[0] < 1) gridN[0] = 1;
    if (gridN[1] < 1) gridN[1] = 1;
    vector<vector<Vec3d>> completeMesh;

    auto metaGeoSector = VRAnalyticGeometry::create("SectorMetaData");
    metaGeoSector->setLabelParams(0.02, true, true, Color4f(0.5,0.1,0,1), Color4f(1,1,0.5,1));

    ppp = fromLatLongPosition(north+0.5*sectorSize,east+0.5*sectorSize);
    for (int x = 0; x <= gridN[0]; x++) {
        vector<Vec3d> row;
        for (int y = 0; y <= gridN[1]; y++){
            Vec3d pVertexGlobal = fromLatLongPosition(north+x*sectorSize/gridN[0], east+y*sectorSize/gridN[1]);
            Vec3d nChunk = ppp - origin->getFrom();
            nChunk.normalize();
            Vec3d nVertexGlobal = pVertexGlobal - origin->getFrom();
            nVertexGlobal.normalize();
            static int IDID = 700; IDID++;
            //metaGeo->setVector(IDID, Vec3d(pVertexGlobal), Vec3d(nVertexGlobal)*10000, Color3f(0,1,0.5), "", false);


            Vec3d pVertexLocal = pVertexGlobal - ppp;
            Vec3d nVertexLocal = Vec3d(0,1,0);
            //metaGeoSector->setVector(IDID, Vec3d(pVertexLocal), Vec3d(nVertexLocal)*100, Color3f(1,0,0.5), "", false);

            /*
            auto n = north+0.5*sectorSize;
            auto e = east+0.5*sectorSize;
            Vec3d p1 = fromLatLongPosition(north+x*sectorSize/gridN[0], east+y*sectorSize/gridN[1]);
            Vec3d p2 = ppp;
            Vec3d d = p1-p2;
            auto dirEast = fromLatLongEast(n, e);
            auto dirNorth = fromLatLongNorth(n, e);
            double u = d.dot( dirEast );
            double v = d.dot( dirNorth );
*/
            //auto planetPos = fromLatLongPose(north+x*sectorSize/gridN[0], east+y*sectorSize/gridN[1])->pos();
            row.push_back(pVertexLocal);
            //row.push_back( Vec3d(u, 0, v) );
/*
            Vec3d no2 = fromLatLongNormal(north+x*sectorSize/gridN[0], east+y*sectorSize/gridN[1]);
            //Vec3d n2 = Vec3d(0,1,0);
            Vec3d p = fromLatLongPosition(north+x*sectorSize/gridN[0], east+y*sectorSize/gridN[1]);
            Vec3d p3 = fromLatLongPosition(north+x*sectorSize/gridN[0], east+y*sectorSize/gridN[1]) - ppp;

            static int IDID = 700; IDID++;//metaGeo->getNewID(); // TODO
            auto nma = toString(IDID);
            metaGeo->setVector(IDID, Vec3d(p), Vec3d(no2)*10000, Color3f(0,1,0.5), "", false);
            metaGeoSector->setVector(IDID, Vec3d(p2), Vec3d(no2)*10000, Color3f(1,0,0.5), "", false);*/
        }
        completeMesh.push_back(row);
    }

    generator->getTerrain()->setMeshTer(completeMesh);
    generator->addChild(metaGeoSector);

    generator->getTerrain()->setParameters( size, 2, 1);
    return generator;
}

VRWorldGeneratorPtr VRPlanet::getSector( double north, double east ) {
    auto sid = toSID(north, east);
    if (sectors.count(sid)) return sectors[sid];
    return 0;
}

void VRPlanet::updateVisSectors(double north, double east){
    return;
    auto pos00 = fromLatLongPose(north+0.5*sectorSize, east+0.5*sectorSize);
    auto pos0 = fromLatLongPose(north, east)->pos();
    auto pos1 = fromLatLongPose(north+sectorSize, east)->pos();
    auto pos2 = fromLatLongPose(north, east+sectorSize)->pos();
    auto pos3 = fromLatLongPose(north+sectorSize, east+sectorSize)->pos();
    //cout << "  POSITIONS: " << toString(pos00)  << " ------ " << toString(pos0) << " -- " << toString(pos1) << " -- " << toString(pos2) << " -- " << toString(pos3) << endl;
    return;
}

vector<VRWorldGeneratorPtr> VRPlanet::getSectors() {
    vector<VRWorldGeneratorPtr> res;
    for (auto s : sectors) res.push_back(s.second);
    return res;
}

void VRPlanet::setupMaterial(string texture, bool isLit) { sphereMat->setTexture(texture); setLit(isLit); }
VRMaterialPtr VRPlanet::getMaterial() { return sphereMat; }
void VRPlanet::setLit(bool b) { sphereMat->setShaderParameter("isLit", b?1:0); }

int VRPlanet::addPin( string label, double north, double east, double length ) {
    if (!metaGeo) {
        metaGeo = VRAnalyticGeometry::create("PlanetMetaData");
        metaGeo->setLabelParams(0.02, true, true, Color4f(0.5,0.1,0,1), Color4f(1,1,0.5,1));
        origin->addChild(metaGeo);
    }

    Vec3d n = fromLatLongNormal(north, east);
    Vec3d p = fromLatLongPosition(north, east);
    static int ID = -1; ID++;//metaGeo->getNewID(); // TODO
    metaGeo->setVector(ID, Vec3d(p), Vec3d(n)*length, Color3f(1,1,0.5), label, true);
    return ID;
}

void VRPlanet::remPin(int ID) {}// metaGeo->remove(ID); } // TODO


// shader --------------------------

string VRPlanet::surfaceVP =
"#version 120\n"
GLSL(
varying vec3 tcs;
varying vec3 normal;
varying vec4 position;

attribute vec4 osg_Vertex;
attribute vec4 osg_Normal;
attribute vec4 osg_Color;
attribute vec3 osg_MultiTexCoords0;

void main( void ) {
    tcs = osg_Normal.xyz;
    normal = gl_NormalMatrix * osg_Normal.xyz;
    position = gl_ModelViewMatrix*osg_Vertex;
    gl_Position = gl_ModelViewProjectionMatrix*osg_Vertex;
}
);


string VRPlanet::surfaceFP =
"#version 120\n"
GLSL(
uniform sampler2D tex;
uniform int isLit;

varying vec3 tcs;
varying vec3 normal;
varying vec4 position;

const float pi = 3.1415926;

vec4 color;

void applyLightning() {
	vec3 n = normalize( normal);
	vec3  light = normalize( gl_LightSource[0].position.xyz - position.xyz ); // point light
	float NdotL = max(dot( n, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot(n, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	gl_FragColor = diffuse + ambient;// + specular;
	//gl_FragColor = ambient + diffuse + specular;
	//gl_FragColor = vec4(gl_LightSource[0].position.xyz,1);
}

void main( void ) {
	float r = length(tcs);
	float v = 1.0 - acos( tcs.y / r )/pi;
	float u0 = 1 - atan(abs(tcs.z), tcs.x)/pi;
	float u1 = 0.5 - atan(tcs.z, tcs.x)/pi*0.5;
	float u2 = 0.0 - atan(-tcs.z, -tcs.x)/pi*0.5;
	vec4 c1 = texture2D(tex, vec2(u1,v));
	vec4 c2 = texture2D(tex, vec2(u2,v));
	float s = clamp( (tcs.x+1)*0.3, 0, 1 );
	color = mix(c2, c1, u0);
	if (isLit != 0) applyLightning();
	else gl_FragColor = color;
}
);





