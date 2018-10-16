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
    p->invert();
    origin->setPose(p);

    lod->hide(); // TODO: work around due to problems with intersection action!!

    for (auto s : sectors) {
        auto sector = s.second;
        Vec2i p = s.first;
        addChild(sector);
        sector->setIdentity();
        Vec2d size = sector->getTerrain()->getSize();
        float X = p[1]*sectorSize-east;
        float Y = p[0]*sectorSize-north;
        cout << "VRPlanet::localize " << X << " " << X*size[0] << endl;
        sector->translate(Vec3d(X*size[0]/sectorSize, 0, Y*size[1]/sectorSize));
    }

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
    if (local) {
        auto m = origin->getWorldMatrix();
        m.invert();
        m.mult(p,p);
    }

    Vec3d n = Vec3d(p);
    n.normalize();
    double cT = n[1];
    double north = toDeg(acos(cT));
    double east = toDeg(atan2(-n[2], n[0]));
    return Vec2d(-north+90, east);
}

void VRPlanet::rebuild() {
    // sphere material
    if (!sphereMat) {
        sphereMat = VRMaterial::create("planet");
        sphereMat->setVertexShader(surfaceVP, "planet surface VS");
        sphereMat->setFragmentShader(surfaceFP, "planet surface FS");
    }

    // spheres
    if (lod) lod->destroy();
    lod = VRLod::create("planetLod");
    auto addLod = [&](int i, double d) {
        auto s = VRGeometry::create("sphere");
        s->setPrimitive("Sphere " + toString(radius)+" "+toString(i));
        s->setMaterial(sphereMat);
        lod->addChild(s);
        lod->addDistance(d);
    };

    origin = VRTransform::create("origin");
    addChild(origin);
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

void VRPlanet::setParameters( double r, double s ) { radius = r; sectorSize = s; rebuild(); }

VRWorldGeneratorPtr VRPlanet::addSector( double north, double east ) {
    auto generator = VRWorldGenerator::create();
    auto sid = toSID(north, east);
    sectors[sid] = generator;
    anchor->addChild(generator);
    generator->setPlanet(ptr(), Vec2d(east, north));
    generator->setPose( fromLatLongPose(north+0.5*sectorSize, east+0.5*sectorSize) );

    Vec2d size = fromLatLongSize(north, east, north+sectorSize, east+sectorSize);
    generator->getTerrain()->setParameters( size, 2, 1);
    return generator;
}

VRWorldGeneratorPtr VRPlanet::getSector( double north, double east ) {
    auto sid = toSID(north, east);
    if (sectors.count(sid)) return sectors[sid];
    return 0;
}

VRMaterialPtr VRPlanet::getMaterial() { return sphereMat; }

int VRPlanet::addPin( string label, double north, double east, double length ) {
    Vec3d n = fromLatLongNormal(north, east);
    Vec3d p = fromLatLongPosition(north, east);
    static int ID = -1; ID++;//metaGeo->getNewID(); // TODO
    metaGeo->setVector(ID, Vec3d(p), Vec3d(n)*length, Color3f(1,1,0.5), label);
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

varying vec3 tcs;
varying vec3 normal;
varying vec4 position;

const float pi = 3.1415926;

vec4 color;

void applyLightning() {
	vec3 n = normal;
	vec3  light = normalize( gl_LightSource[0].position.xyz - position.xyz ); // point light
	float NdotL = max(dot( n, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot(n, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	gl_FragColor = diffuse + specular;
	//gl_FragColor = ambient + diffuse + specular;
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
	//applyLightning(); // TODO
	gl_FragColor = color;
}
);





