#include "VRPlanet.h"
#include "VRTerrain.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRLod.h"
#include "core/tools/VRAnalyticGeometry.h"
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

Vec3d VRPlanet::fromLatLongNormal(double north, double east) {
    north = -north+90;
	double sT = sin(toRad(north));
	double sP = sin(toRad(east));
	double cT = cos(toRad(north));
	double cP = cos(toRad(east));
	return Vec3d(sT*cP, cT, -sT*sP);
}

Vec3d VRPlanet::fromLatLongPosition(double north, double east) { return fromLatLongNormal(north, east)*radius; }
Vec3d VRPlanet::fromLatLongEast(double north, double east) { return fromLatLongNormal(0, east+90); }
Vec3d VRPlanet::fromLatLongNorth(double north, double east) { return fromLatLongNormal(north+90, east); }

Vec2d VRPlanet::fromLatLongSize(double north1, double east1, double north2, double east2) {
    auto n = (north1+north2)*0.5;
    auto e = (east1+east2)*0.5;
    Vec3d p1 = fromLatLongPosition(north1, east1);
    Vec3d p2 = fromLatLongPosition(north2, east2);
    addPin("P1", north1, east1);
    addPin("P2", north2, east2);
    Vec3d d = p2-p1;
    auto dirEast = fromLatLongEast(n, e);
    auto dirNorth = fromLatLongNorth(n, e);
    double u = d.dot( dirEast );
    double v = d.dot( dirNorth );

    d = fromLatLongPosition(north2, east2) - fromLatLongPosition(north2, east1);
    metaGeo->setVector(101, Vec3d(p1), Vec3d(d), Color3f(1,1,0), "D");
    metaGeo->setVector(102, Vec3d(p1), Vec3d(dirEast*u), Color3f(0,1,1), "E");
    metaGeo->setVector(103, Vec3d(p1), Vec3d(dirNorth*v), Color3f(0,1,1), "S");

    return Vec2d(abs(u),abs(v));
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
        s->setPrimitive("Sphere", toString(radius)+" "+toString(i));
        s->setMaterial(sphereMat);
        lod->addChild(s);
        lod->addDistance(d);
    };

    addChild(lod);
    anchor = VRObject::create("lod0");
    lod->addChild( anchor );
    addLod(5,radius*1.25);
    addLod(4,radius*2.0);
    addLod(3,radius*5.0);

    // init meta geo
    if (!metaGeo) {
        metaGeo = VRAnalyticGeometry::create("PlanetMetaData");
        metaGeo->setLabelParams(0.1, true, true, Color4f(1,1,1,1), Color4f(1,0,0,1));
        addChild(metaGeo);
    }
}

void VRPlanet::setParameters( double r ) { radius = r; rebuild(); }

VRTerrainPtr VRPlanet::addSector( int north, int east ) {
    auto terrain = VRTerrain::create( toString(north)+"N"+toString(east)+"E" );
    terrain->setPlanet(ptr(), Vec2d(east, north));
    anchor->addChild(terrain);
    sectors[north][east] = terrain;
    terrain->setFrom( Vec3d(fromLatLongPosition(north+0.5, east+0.5)) );
    terrain->setUp( Vec3d(fromLatLongNormal(north+0.5, east+0.5)) );
    terrain->setDir( Vec3d(fromLatLongNorth(north+0.5, east+0.5)) );

    Vec2d size = fromLatLongSize(north, east, north+1, east+1);
    terrain->setParameters( size, 10, 1);
    return terrain;
}

VRTerrainPtr VRPlanet::getSector( double north, double east ) {
    int N = floor(north);
    int E = floor(east);
    if (sectors.count(N)) if (sectors[N].count(E)) return sectors[N][E];
    return 0;
}

VRMaterialPtr VRPlanet::getMaterial() { return sphereMat; }

int VRPlanet::addPin( string label, double north, double east ) {
    Vec3d n = fromLatLongNormal(north, east);
    Vec3d p = fromLatLongPosition(north, east);
    static int ID = -1; ID++;//metaGeo->getNewID(); // TODO
    metaGeo->setVector(ID, Vec3d(p), Vec3d(n)*10000, Color3f(1,0,0), label);
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





