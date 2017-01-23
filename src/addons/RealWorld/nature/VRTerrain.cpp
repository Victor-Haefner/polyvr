#include "VRTerrain.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/geometry/VRGeoData.h"

#define GLSL(shader) #shader

using namespace OSG;

VRTerrain::VRTerrain(string name) : VRGeometry(name) { setupMat(); }
VRTerrain::~VRTerrain() {}
VRTerrainPtr VRTerrain::create(string name) { return VRTerrainPtr( new VRTerrain(name) ); }

void VRTerrain::setParameters( Vec2f s, float r ) { size = s; resolution = r; grid = r*64; setupGeo(); }
void VRTerrain::setMap( VRTexturePtr t ) { tex = t; mat->setTexture(t); }

void VRTerrain::setupGeo() {
    Vec2i gridN = Vec2i(size*1.0/grid);
	Vec2f tcChunk = Vec2f(1.0/gridN[0], 1.0/gridN[1]);

	VRGeoData geo;
    for (int i =0; i < gridN[0]; i++) {
		float px1 = -size[0]*0.5 + i*grid;
		float px2 = px1 + grid;
		float tcx1 = 0 + i*tcChunk[0];
		float tcx2 = tcx1 + tcChunk[0];

		for (int j =0; j < gridN[1]; j++) {
			float py1 = -size[1]*0.5 + j*grid;
			float py2 = py1 + grid;
			float tcy1 = 0 + j*tcChunk[1];
			float tcy2 = tcy1 + tcChunk[1];

			geo.pushVert(Vec3f(px1,0,py1), Vec3f(0,1,0), Vec2f(tcx1,tcy1));
			geo.pushVert(Vec3f(px1,0,py2), Vec3f(0,1,0), Vec2f(tcx1,tcy2));
			geo.pushVert(Vec3f(px2,0,py2), Vec3f(0,1,0), Vec2f(tcx2,tcy2));
			geo.pushVert(Vec3f(px2,0,py1), Vec3f(0,1,0), Vec2f(tcx2,tcy1));
			geo.pushQuad();
		}
	}

	geo.apply(ptr());
	setType(GL_PATCHES);
	setPatchVertices(4);
	setMaterial(mat);
}

void VRTerrain::setupMat() {
    Vec4f w = Vec4f(1,1,1,1);
    VRTextureGenerator tg;
	tg.setSize(Vec3i(400,400,1),true);
	tg.add("Perlin", 1.0, w*0.97, w);
	tg.add("Perlin", 1.0/2, w*0.95, w);
	tg.add("Perlin", 1.0/4, w*0.85, w);
	tg.add("Perlin", 1.0/8, w*0.8, w);
	tg.add("Perlin", 1.0/16, w*0.7, w);
	tg.add("Perlin", 1.0/32, w*0.5, w);
	tex = tg.compose(0);

	mat = VRMaterial::create("terrain");
	mat->setWireFrame(0);
	mat->setLineWidth(1);
	mat->setVertexShader(vertexShader, "terrainVS");
	mat->setFragmentShader(fragmentShader, "terrainFS");
	mat->setTessControlShader(tessControlShader, "terrainTCS");
	mat->setTessEvaluationShader(tessEvaluationShader, "terrainTES");
	mat->setTexture(tex);
}



string VRTerrain::vertexShader =
"#version 120\n"
GLSL(
attribute vec4 osg_Vertex;
attribute vec2 osg_MultiTexCoord0;

void main(void) {
    gl_TexCoord[0] = vec4(osg_MultiTexCoord0,0.0,0.0);
	gl_Position = osg_Vertex;
}
);

string VRTerrain::fragmentShader =
"#version 400 compatibility\n"
GLSL(
uniform sampler2D tex;
const vec2 size = vec2(0.2,0.0);
const ivec3 off = ivec3(-1,0,1);
const vec3 light = vec3(-1,-1,-0.5);

vec3 getNormal() {
	vec2 tc = gl_TexCoord[0].xy;
    float s11 = texture(tex, tc).a;
    float s01 = textureOffset(tex, tc, off.xy).a;
    float s21 = textureOffset(tex, tc, off.zy).a;
    float s10 = textureOffset(tex, tc, off.yx).a;
    float s12 = textureOffset(tex, tc, off.yz).a;

    vec3 va = normalize(vec3(size.xy,s21-s01));
    vec3 vb = normalize(vec3(size.yx,s12-s10));
    vec3 n = cross(va,vb);
	return n;
}

vec3 mixColor(vec3 c1, vec3 c2, float t) {
	t = clamp(t, 0.0, 1.0);
	return mix(c1, c2, t);
}

vec3 getColor() {
	/*vec3 green = vec3(0.2,0.5,0.2);
	vec3 black = vec3(0.0);
	vec3 brown = vec3(0.6,0.5,0.2);
	vec3 grey = vec3(0.7);
	vec3 white = vec3(1.0);

	float h = texture2D(tex, gl_TexCoord[0].xy).a;
	if (h > 1.2) return mixColor(grey, white, 10*(h-1.2));
	if (h > 1.0 && h <= 1.2) return mixColor(brown, grey, 5*(h-1.0));
	if (h > 0.8 && h <= 1.0) return mixColor(green, brown, 5*(h-0.8));
	if (h <= 0.8) return mixColor(black, green, 1.25*h);
	return black;*/

	return texture2D(tex, gl_TexCoord[0].xy).rgb;
}

void main( void ) {
	vec3 n = getNormal();
	vec3 c = getColor();

	// diffuse lightning
	vec4 aCol = vec4(c * 0.4, 1.0f);
	float dFac = dot(normalize(n), -normalize(light));
	vec4 dCol = vec4(c * 0.6 * dFac, 1.0f);
	gl_FragColor = aCol+dCol;
}
);

string VRTerrain::tessControlShader =
"#version 400 compatibility\n"
"#extension GL_ARB_tessellation_shader : enable\n"
GLSL(
layout(vertices = 4) out;
out vec3 tcPosition[];
out vec2 tcTexCoords[];
)
"\n#define ID gl_InvocationID\n"
GLSL(
void main() {
    tcPosition[ID] = gl_in[ID].gl_Position.xyz;
    tcTexCoords[ID] = gl_in[ID].gl_TexCoord[0].xy;

    if (ID == 0) {
		vec4 pos = gl_ModelViewProjectionMatrix * vec4(tcPosition[ID], 1.0);
		float D = length(pos.xyz);
		int p = int(5.0/D);
		int res = int(pow(2,p));
		res = clamp(res, 1, 64);

        gl_TessLevelInner[0] = res;
        gl_TessLevelInner[1] = res;

        gl_TessLevelOuter[0] = 64;
        gl_TessLevelOuter[1] = 64;
        gl_TessLevelOuter[2] = 64;
        gl_TessLevelOuter[3] = 64;
    }
}
);

string VRTerrain::tessEvaluationShader =
"#version 400 compatibility\n"
"#extension GL_ARB_tessellation_shader : enable\n"
GLSL(
layout( quads ) in;
in vec3 tcPosition[];
in vec2 tcTexCoords[];

uniform sampler2D texture;

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec2 ta = mix(tcTexCoords[0], tcTexCoords[1], u);
    vec2 tb = mix(tcTexCoords[3], tcTexCoords[2], u);
    vec2 tc = mix(ta, tb, v);
    gl_TexCoord[0] = vec4(tc.x, tc.y, 1.0, 1.0);

    vec3 a = mix(tcPosition[0], tcPosition[1], u);
    vec3 b = mix(tcPosition[3], tcPosition[2], u);
    vec3 tePosition = mix(a, b, v);
    tePosition.y = 0.4*texture2D(texture, gl_TexCoord[0].xy).a;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(tePosition, 1);
}
);

