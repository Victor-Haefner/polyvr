#include "VRPlantMaterial.h"

#define GLSL(shader) #shader

using namespace OSG;

VRPlantMaterial::VRPlantMaterial() : VRMaterial("plants") {
    composeShader();
}

VRPlantMaterial::~VRPlantMaterial() {}

VRPlantMaterialPtr VRPlantMaterial::create() { return VRPlantMaterialPtr( new VRPlantMaterial() ); }

void VRPlantMaterial::composeShader() {
    string vshrd;
    vshrd += vShrdHead;
    vshrd += vShrdEnd;

    string fshrd;
    fshrd += fShrdHead;
    fshrd += lightning;
    fshrd += fShrdEnd;

    setVertexShader(vshrd, "plantVS");
    setFragmentShader(fshrd, "plantFS");
}



// --- vertex chunks --- //

string VRPlantMaterial::vShrdHead = GLSL(
varying vec3 vnrm;
varying vec3 vcol;
varying vec2 vtcs;
varying vec3 vpos;
attribute vec4 osg_Vertex;
attribute vec3 osg_Normal;
attribute vec3 osg_Color;
attribute vec2 osg_MultiTexCoord0;

void main( void ) {
	vnrm = normalize( gl_NormalMatrix * osg_Normal );
	vcol = osg_Color;
	vtcs = osg_MultiTexCoord0;
    vpos = osg_Vertex;
);

string VRPlantMaterial::vShrdEnd = GLSL(
    gl_Position = gl_ModelViewProjectionMatrix * osg_Vertex;
}
);


// --- fragment chunks --- //

string VRPlantMaterial::lightning = GLSL(
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
);

string VRPlantMaterial::fShrdHead = GLSL(
uniform sampler2D tex;
varying vec2 vtcs;
varying vec3 vnrm;
varying vec3 vcol;
vec4 color;
vec3 normal;
);

string VRPlantMaterial::fShrdEnd = GLSL(
void main( void ) {
	normal = vnrm;
	color = texture2D(tex,vtcs);
	if (color.a < 0.3) discard;
	applyLightning();
}
);

