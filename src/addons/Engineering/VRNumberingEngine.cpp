#include "VRNumberingEngine.h"

#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#ifndef WITHOUT_PANGO_CAIRO
#include "core/tools/VRText.h"
#endif
#include "core/utils/toString.h"

#define GLSL(shader) #shader

using namespace OSG;

VRNumberingEngine::VRNumberingEngine(string name) : VRGeometry(name) {
    group g;
    groups.push_back(g);

    mat = VRMaterial::get("NumbEngMat");
    mat->setVertexShader(vp, "numbEngVS");
    mat->setFragmentShader(fp, "numbEngFS");
    mat->setGeometryShader(gp, "numbEngGS");
    mat->setPointSize(5);
    setMaterial(mat);
    updateTexture();

    setSize(0.2);
    setBillboard(false);
    setOnTop(false);

    data = VRGeoData::create();
}

void VRNumberingEngine::clear() { data->reset(); }

VRNumberingEnginePtr VRNumberingEngine::create(string name) { return shared_ptr<VRNumberingEngine>(new VRNumberingEngine(name) ); }
VRNumberingEnginePtr VRNumberingEngine::ptr() { return static_pointer_cast<VRNumberingEngine>( shared_from_this() ); }

bool VRNumberingEngine::checkUIn(int grp) {
    if (grp < 0 || grp > (int)groups.size()) return true;
    return false;
}

bool VRNumberingEngine::checkUIn(int i, int grp) {
    if (grp < 0 || grp > (int)groups.size()) return true;
    if (i < 0 || i > (int)data->size()) return true;
    return false;
}

void VRNumberingEngine::resize(int N) {
    int pN = data->size();
    if (N < pN) return;
    for (int i=pN; i<N; i++) {
        data->pushVert(Pnt3d(), Vec3d());
        data->pushPoint();
    }
    data->apply( ptr() );
}

void VRNumberingEngine::set(int i, Vec3d p, float f, int d, int grp) {
    if (checkUIn(i,grp)) return;
    resize(i+1);
    float f1 = floor(f);
    float f2 = f-f1;
    string sf2 = toString(f2);
    if (sf2.size() > 2) sf2 = sf2.substr(2,sf2.size()-1);
    f2 = toFloat(sf2);
    data->setVert(i, p, Vec3d(f1,grp,f2));
}

int VRNumberingEngine::add(Vec3d p, float f, int d, int grp) {
    int pN = data->size();
    set(pN, p, f, d, grp);
    return pN;
}

void VRNumberingEngine::setPrePost(int grp, string pre, string post) {
    if (checkUIn(grp)) return;
    groups[grp].pre = pre;
    groups[grp].post = post;
}

int VRNumberingEngine::addPrePost(string pre, string post) {
    group g;
    g.pre = pre; g.post = post;
    groups.push_back(g);
    return groups.size()-1;
}

void VRNumberingEngine::setOnTop(bool b) { mat->setShaderParameter("onTop", Real32(b)); }
void VRNumberingEngine::setSize(float f) { mat->setShaderParameter("size", Real32(f)); }
void VRNumberingEngine::setBillboard(bool b) { mat->setShaderParameter("doBillboard", Real32(b)); }

void VRNumberingEngine::updateTexture() {
#ifndef WITHOUT_PANGO_CAIRO
    string txt = "0123456789.";
    //for (auto g : groups) txt += "\n+g.pre+"\n"+g.post;
    auto img = VRText::get()->create(txt, "Mono.ttf", 20, 3, Color4f(0,0,0,1), Color4f(0,0,0,0) );
    mat->setTexture(img);
#endif
}

string VRNumberingEngine::vp =
"#version 120\n"
GLSL(
uniform float onTop;
varying mat4 model;
varying vec3 normal;

attribute vec4 osg_Vertex;
attribute vec4 osg_Normal;

void main( void ) {
    model = gl_ModelViewProjectionMatrix;
    gl_Position = model*osg_Vertex;
    normal = osg_Normal.xyz;
    if (onTop > 0.0) gl_Position.z -= 0.5;
}
);

string VRNumberingEngine::fp =
"#version 120\n"
GLSL(
uniform sampler2D texture;

varying vec2 texCoord;

void main( void ) {
  gl_FragColor = texture2D(texture, texCoord);
}
);

string VRNumberingEngine::gp =
"#version 150\n"
"#extension GL_EXT_geometry_shader4 : enable\n"
GLSL(
layout (points) in;
layout (triangle_strip, max_vertices=60) out;

uniform float doBillboard;
uniform float size;
uniform vec2 OSGViewportSize;
in mat4 model[];
in vec3 normal[];
out vec2 texCoord;

void emitVertex(in vec4 p, in vec2 tc) {
 gl_Position = p;
 texCoord = tc;
 EmitVertex();
}

void emitQuad(in float offset, in vec4 tc) {
 float sx = 0.5*size;
 float sy = size;
 float ox = 2*sx*offset;
 vec4 p1;
 vec4 p2;
 vec4 p3;
 vec4 p4;

 if (doBillboard < 0.5) {
  p1 = gl_PositionIn[0]+model[0]*vec4(-sx+ox,-sy,0,0);
  p2 = gl_PositionIn[0]+model[0]*vec4(-sx+ox, sy,0,0);
  p3 = gl_PositionIn[0]+model[0]*vec4( sx+ox, sy,0,0);
  p4 = gl_PositionIn[0]+model[0]*vec4( sx+ox,-sy,0,0);
 } else {
  float a = OSGViewportSize.y/OSGViewportSize.x;
  p1 = gl_PositionIn[0]+vec4(-sx*a+ox*a,-sy,0,0);
  p2 = gl_PositionIn[0]+vec4(-sx*a+ox*a, sy,0,0);
  p3 = gl_PositionIn[0]+vec4( sx*a+ox*a, sy,0,0);
  p4 = gl_PositionIn[0]+vec4( sx*a+ox*a,-sy,0,0);
 }

 emitVertex(p1, vec2(tc[0], tc[2]));
 emitVertex(p2, vec2(tc[0], tc[3]));
 emitVertex(p3, vec2(tc[1], tc[3]));
 EndPrimitive();
 emitVertex(p1, vec2(tc[0], tc[2]));
 emitVertex(p3, vec2(tc[1], tc[3]));
 emitVertex(p4, vec2(tc[1], tc[2]));
 EndPrimitive();
}

void emitDot(in float p) {
 float f = 0.0727;
 int d = 10;
 emitQuad(p, vec4(d*f,d*f+f, 0,1));
}

void emitDigit(in int d, in float p) {
 float f = 0.0727;
 emitQuad(p, vec4(d*f,d*f+f, 0,1));
}

void emitNumber(in float n1, in float n2, in int N) {
 int d=0;
 int k1 = int(n1);
 int k2 = int(n2);
 int i=0;
 int first = 1;
 float p = 0;

 if (k2 == 0) first = 0;

 while(true) {
   if (first == 1) {
     d = k2%10;
     k2 = int(k2*0.1);
     p = -i*1.5;
     emitDigit(d, p);
   } else {
     d = k1%10;
     k1 = int(k1*0.1);
     p = -i*1.5;
     emitDigit(d, p);
   }

   if (first == 1) {
     if (k2 == 0 || i == N) {
       i++;
       p = -i*1.5;
       emitDot(p);
       first = 2;
       k2 = 0;
     }
   }

   i++;
   if (k1 == 0 && k2 == 0) return;
   if (i > 10) return;
 }
}

void main() {
    emitNumber(normal[0][0], normal[0][2], 1);
}
);








