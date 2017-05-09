#include "VRAsphalt.h"

#define GLSL(shader) #shader

using namespace OSG;

VRAsphalt::VRAsphalt() : VRMaterial("asphalt") {
    //setVertexShader(asphalt_vp);
    //setFragmentShader(asphalt_fp);
    //setFragmentShader(asphalt_dfp, true);
    clearTransparency();
    setShininess(128);
}

VRAsphalt::~VRAsphalt() {}

VRAsphaltPtr VRAsphalt::create() { return VRAsphaltPtr( new VRAsphalt() ); }


string VRAsphalt::asphalt_vp =
"#version 400 compatibility"
GLSL(

in vec4 osg_Vertex;
in vec2 osg_MultiTexCoord0;
in vec2 osg_MultiTexCoord1;

out vec4 position;
out vec2 tc1;
flat out int rID;

void main(void) {
	position = gl_ModelViewMatrix * osg_Vertex;
	gl_Position = gl_ModelViewProjectionMatrix * osg_Vertex;
	tc1 = osg_MultiTexCoord0;
	rID = int(osg_MultiTexCoord1.x);
}
);


string VRAsphalt::asphalt_fp =
"#version 400 compatibility"
"#extension GL_ARB_texture_rectangle : require"
"#extension GL_ARB_texture_rectangle : enable"
GLSL(

void main(void) {
	uv = tc1.xy;
	roadData = getData(rID, 0);
	noise = texture(texNoise, uv*0.2).r;
	computeNormal();
	asphalt();
	doPaths();
	applyMud();
	//color = texture(texMarkings, uv);
	//color = texture(texMud, uv);
	//color = texture(texNoise, uv);

	norm = gl_NormalMatrix * norm;
    computeDepth(doLine,doTrack);

    applyBlinnPhong();
}
);


string VRAsphalt::asphalt_dfp =
"#version 400 compatibility"
"#extension GL_ARB_texture_rectangle : require"
"#extension GL_ARB_texture_rectangle : enable"
GLSL(
);

