#include "VRPlanet.h"
#include "VRTerrain.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRLod.h"
#include "core/utils/toString.h"

#define GLSL(shader) #shader

const float pi = 3.14159265359;

using namespace OSG;

VRPlanet::VRPlanet(string name) : VRTransform(name) {}
VRPlanet::~VRPlanet() {}
VRPlanetPtr VRPlanet::create(string name) {
    auto p = VRPlanetPtr( new VRPlanet(name) );
    p->rebuild();
    return p;
}

float VRPlanet::toRad(float deg) { return pi*deg/180; }
float VRPlanet::toDeg(float rad) { return 180*rad/pi; }

Vec3f VRPlanet::fromLatLongNormal(float north, float east) {
    north = -north+90;
	float sT = sin(toRad(north));
	float sP = sin(toRad(east));
	float cT = cos(toRad(north));
	float cP = cos(toRad(east));
	Vec3f N(sT*cP, cT, -sT*sP);
    return N;
}

Vec3f VRPlanet::fromLatLongPosition(float north, float east) { return fromLatLongNormal(north, east)*radius; }
Vec3f VRPlanet::fromLatLongEast(float north, float east) { return fromLatLongNormal(north, east+90); }
Vec3f VRPlanet::fromLatLongSouth(float north, float east) { return fromLatLongNormal(north+90, east); }
Vec2f VRPlanet::fromLatLongSize(float north1, float east1, float north2, float east2) {
    auto n = (north1+north2)*0.5;
    auto e = (east1+east2)*0.5;
    Vec3f p1 = fromLatLongPosition(north1, east1);
    Vec3f p2 = fromLatLongPosition(north2, east2);
    Vec3f d = p2-p1;
    float u = d.dot( fromLatLongEast(n, e) );
    float v = d.dot( fromLatLongSouth(n, e) );
    return Vec2f(abs(u),abs(v));
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
    auto addLod = [&](int i, float d) {
        auto s = VRGeometry::create("sphere");
        s->setPrimitive("Sphere", toString(radius)+" "+toString(i));
        s->setMaterial(sphereMat);
        lod->addChild(s);
        lod->addDistance(d);
    };

    addChild(lod);
    anchor = VRObject::create("lod0");
    lod->addChild( anchor );
    addLod(4,radius*1.25);
    addLod(3,radius*1.50);
    addLod(2,radius*1.75);
}

void VRPlanet::setParameters( float r ) { radius = r; rebuild(); }

VRTerrainPtr VRPlanet::addSector( int north, int east ) {
    auto terrain = VRTerrain::create( toString(north)+"N"+toString(east)+"E" );
    anchor->addChild(terrain);
    sectors[north][east] = terrain;
    terrain->setFrom( fromLatLongPosition(north, east) );
    terrain->setUp( fromLatLongNormal(north, east) );
    terrain->setDir( fromLatLongSouth(north, east) );

    Vec2f size = fromLatLongSize(north-0.5, east-0.5, north+0.5, east+0.5);
    terrain->setParameters( size, 1, 0.001);
    return terrain;
}

VRMaterialPtr VRPlanet::getMaterial() { return sphereMat; }





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
    position = gl_ModelViewProjectionMatrix*osg_Vertex;
    gl_Position = gl_ModelViewProjectionMatrix*osg_Vertex;
}
);


string VRPlanet::surfaceFP =
"#version 120\n"
GLSL(
uniform sampler2D tex;

in vec3 tcs;
in vec3 normal;
in vec4 position;

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
	float u = 0.5 - atan( tcs.z, tcs.x )*0.5/pi;
	float v = -acos( tcs.y / r )/pi;
	color = texture2D(tex, vec2(u,v));
	//applyLightning(); // TODO
	gl_FragColor = color;
}
);





