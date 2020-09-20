#include "VRMoleculeMat.h"
#include "core/objects/geometry/VRGeometry.h"

#define GLSL(shader) #shader

using namespace OSG;

VRMoleculeMat::VRMoleculeMat() {
    mat1 = VRMaterial::get("atoms");
    mat1->setPointSize(40);
    mat1->setLit(false);
    mat1->setVertexShader(a_vp, "moleculesVS");
    mat1->setFragmentShader(a_fp, "moleculesFS");
    mat1->setGeometryShader(a_gp, "moleculesGS");

    mat2 = VRMaterial::get("molecule_bonds");
    mat2->setLineWidth(5);
    mat2->setLit(false);
    mat2->setVertexShader(b_vp, "moleculeBondsVS");
    mat2->setFragmentShader(b_fp, "moleculeBondsFS");
    mat2->setGeometryShader(b_gp, "moleculeBondsGS");

    mat3 = VRMaterial::get("coords");
    mat3->setLineWidth(2);
    mat3->setLit(false);
}

VRMoleculeMat::~VRMoleculeMat() {;}

VRMoleculeMatPtr VRMoleculeMat::create() { return VRMoleculeMatPtr( new VRMoleculeMat() ); }

VRMaterialPtr VRMoleculeMat::getAtomsMaterial() { return mat1; }
VRMaterialPtr VRMoleculeMat::getBondsMaterial() { return mat2; }
VRMaterialPtr VRMoleculeMat::getCoordsMaterial() { return mat3; }

void VRMoleculeMat::apply(VRGeometryPtr atoms, VRGeometryPtr bonds) {
    atoms->setMaterial(mat1);
    bonds->setMaterial(mat2);
}

string VRMoleculeMat::a_fp =
"#version 120\n"
GLSL(
in vec2 texCoord;
in vec4 Color;

void main( void ) {
	vec2 p = 2* ( vec2(0.5, 0.5) - texCoord );
	float r = sqrt(dot(p,p));
	if (r > 1.0) discard;

	float f = 1.2 - (1.0-sqrt(1.0-r))/(r);
	vec4 amb = vec4(0.2);
	gl_FragColor = Color*f + amb;
}
);

string VRMoleculeMat::a_vp =
"#version 120\n"
GLSL(
varying vec4 color;
varying vec3 normal;

attribute vec4 osg_Vertex;
attribute vec4 osg_Normal;
attribute vec4 osg_Color;

void main( void ) {
    color = osg_Color;
    normal = osg_Normal.xyz;
    gl_Position = gl_ModelViewProjectionMatrix*osg_Vertex;
}
);

string VRMoleculeMat::a_gp =
"#version 150\n"
"#extension GL_EXT_geometry_shader4 : enable\n"
GLSL(
layout (points) in;
layout (triangle_strip, max_vertices=6) out;

uniform vec2 OSGViewportSize;

in vec4 color[];
in vec3 normal[];
out vec2 texCoord;
out vec4 Color;

void emitVertex(in vec4 p, in vec2 tc) {
	gl_Position = p;
	texCoord = tc;
	EmitVertex();
}

void emitQuad(in float s, in vec4 tc) {
	vec4 p = gl_PositionIn[0];

	float a = OSGViewportSize.y/OSGViewportSize.x;

	vec4 u = vec4(s*a,0,0,0);
	vec4 v = vec4(0,s,0,0);

	vec4 p1 = p -u -v;
	vec4 p2 = p -u +v;
	vec4 p3 = p +u +v;
	vec4 p4 = p +u -v;

	emitVertex(p1, vec2(tc[0], tc[2]));
	emitVertex(p2, vec2(tc[0], tc[3]));
	emitVertex(p3, vec2(tc[1], tc[3]));
	EndPrimitive();
	emitVertex(p1, vec2(tc[0], tc[2]));
	emitVertex(p3, vec2(tc[1], tc[3]));
	emitVertex(p4, vec2(tc[1], tc[2]));
	EndPrimitive();
}

void main() {
	Color = color[0];
	emitQuad(normal[0][1], vec4(0,1,0,1));
}
);

string VRMoleculeMat::b_fp =
"#version 120\n"
GLSL(
in vec2 texCoord;
in vec4 Color;

void main( void ) {
	float r = texCoord.y;
	r = 2.8*abs(r - floor(r) - 0.5);
	if (r > 1.0) discard;

	float f = 1.2 - (1.0-sqrt(1.0-r))/(r);
	vec4 amb = vec4(0.2);
	gl_FragColor = Color*f + amb;
}
);

string VRMoleculeMat::b_vp =
"#version 120\n"
GLSL(
varying vec3 normal;

attribute vec4 osg_Vertex;
attribute vec4 osg_Normal;
attribute vec4 osg_Color;

void main( void ) {
    normal = osg_Normal.xyz;
    gl_Position = gl_ModelViewProjectionMatrix*osg_Vertex;
}
);

string VRMoleculeMat::b_gp =
"#version 150\n"
"#extension GL_EXT_geometry_shader4 : enable\n"
GLSL(
layout (lines) in;
layout (triangle_strip, max_vertices=6) out;

uniform vec2 OSGViewportSize;

in vec3 normal[];
out vec2 texCoord;
out vec4 Color;

void emitVertex(in vec4 p, in vec2 tc) {
	gl_Position = p;
	texCoord = tc;
	EmitVertex();
}

void emitQuad(in float s, in float f, in vec4 tc) {
	vec4 pl1 = gl_PositionIn[0];
	vec4 pl2 = gl_PositionIn[1];

	vec3 n2 = normal[1];

	float a = OSGViewportSize.y/OSGViewportSize.x;

	vec4 d = pl2-pl1;
	pl1 += 0.7*n2.y*d;
	pl2 -= 0.7*n2.z*d;

	vec3 x = cross(d.xyz, vec3(0,0,1));
	x.x *= a;
	x = f*normalize(x);
	x.x *= a;
	vec4 v = vec4(x,0);

	vec4 p1 = pl1 -v;
	vec4 p2 = pl1 +v;
	vec4 p3 = pl2 +v;
	vec4 p4 = pl2 -v;

	emitVertex(p1, vec2(tc[0], tc[2]));
	emitVertex(p2, vec2(tc[0], tc[3]));
	emitVertex(p3, vec2(tc[1], tc[3]));
	EndPrimitive();
	emitVertex(p1, vec2(tc[0], tc[2]));
	emitVertex(p3, vec2(tc[1], tc[3]));
	emitVertex(p4, vec2(tc[1], tc[2]));
	EndPrimitive();
}

void main() {
	vec3 n1 = normal[0];
	vec3 n2 = normal[1];
	float b = n2.x;

	Color = vec4(0.5, 0.5, 0.5, 1.0);
	vec4 tc = vec4(0,1,0,1);
	float w = 0.09;
	float k = 1.0;

	if (b > 0.0 && b < 0.15) Color.y += 0.5; /* single */

	if (b > 0.15 && b < 0.25) { /* double */
		Color.x += 0.5;
		Color.y += 0.5;
		w = 1.5*w;
		k = 2.0;
	}

	if (b > 0.25 && b < 0.35) { /* triple */
		Color.x += 0.5;
		w = 2*w;
		k = 3.0;
	}

	if (b > 0.35 && b < 0.45) { /* el pair */
		Color.x += 0.5;
		Color.y += 0.3;
		w = 2*w;
		k = 1.0;
	}

	emitQuad(0.2, w, vec4(0.0, k, 0.0, k));
}
);
