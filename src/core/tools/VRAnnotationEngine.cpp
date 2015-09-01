#include "VRAnnotationEngine.h"

#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/tools/VRText.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>

#define GLSL(shader) #shader

using namespace OSG;
using namespace std;

VRAnnotationEngine::VRAnnotationEngine() : VRGeometry("AnnEng") {
    fg = Vec4f(0,0,0,1);
    bg = Vec4f(1,0,1,0);

    mat = new VRMaterial("AnnEngMat");
    mat->setVertexShader(vp);
    mat->setFragmentShader(fp);
    mat->setGeometryShader(gp);
    mat->setPointSize(5);
    updateTexture();

    setSize(0.2);
    setBillboard(false);
    setOnTop(false);

    clear();
}

void VRAnnotationEngine::clear() {
    OSG::GeoPnt3fPropertyRecPtr pos = OSG::GeoPnt3fProperty::create();
    OSG::GeoVec3fPropertyRecPtr norms = OSG::GeoVec3fProperty::create();
    OSG::GeoUInt32PropertyRecPtr inds = OSG::GeoUInt32Property::create();

    this->pos = pos;
    this->norms = norms;

    setType(GL_POINTS);
    setPositions(pos);
    setNormals(norms);
    setIndices(inds);

    mesh->getLengths()->setValue(0, 0);
    setMaterial(mat);
}

void VRAnnotationEngine::setColor(Vec4f c) { fg = c; updateTexture(); }
void VRAnnotationEngine::setBackground(Vec4f c) { bg = c; updateTexture(); }

bool VRAnnotationEngine::checkUIn(int i) {
    if (i < 0 || i > (int)pos->size()) return true;
    return false;
}

void VRAnnotationEngine::resize(Label& l, Vec3f p, int N) {
    int eN = l.entries.size();
    for (int i=0; i<eN; i++) norms->setValue(Vec3f(), l.entries[i]);

    if (N <= eN) return;
    l.entries.resize(N, 0);
    int pN = pos->size();

    cout << "VRAnnotationEngine::resize " << eN << " " << N << " " << pN << endl;

    mesh->getLengths()->setValue(N-eN+pN, 0);
    for (int i=0; i<N-eN; i++) {
        pos->addValue(p);
        norms->addValue(Vec3f());
        mesh->getIndices()->addValue(pN+i);
        l.entries[eN+i] = pN+i;
    }
}

void VRAnnotationEngine::set(int i, Vec3f p, string s) {
    if (i < 0) return;
    while (i >= (int)labels.size()) labels.push_back(Label());

    int N = ceil(s.size()/3.0); // number of points, 3 chars per point
    auto& l = labels[i];
    resize(l,p,N);

    for (int j=0; j<N; j++) {
        char c[] = {0,0,0};
        for (int k = 0; k<3; k++) {
            uint si = j*3+k;
            if (si < s.size()) c[k] = s[si];
        }
        float f = c[0] + c[1]*256 + c[2]*256*256;
        int k = l.entries[j];
        pos->setValue(p, k);
        norms->setValue(Vec3f(f,0,j), k);
    }
}

void VRAnnotationEngine::setOnTop(bool b) { mat->setShaderParameter("onTop", Real32(b)); }
void VRAnnotationEngine::setSize(float f) { mat->setShaderParameter("size", Real32(f)); }
void VRAnnotationEngine::setBillboard(bool b) { mat->setShaderParameter("doBillboard", Real32(b)); }

void VRAnnotationEngine::updateTexture() {
    string txt;
    for (int i=32; i<128; i++) txt += char(i);
    ImageRecPtr img = VRText::get()->create(txt, "MONO 20", 20, fg*255, bg*255 );
    mat->setTexture(img);
}

string VRAnnotationEngine::vp =
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

string VRAnnotationEngine::fp =
"#version 120\n"
GLSL(
uniform sampler2D texture;

in vec2 texCoord;

void main( void ) {
  //gl_FragColor = vec4(1.0,1.0,0.0,1.0);
  gl_FragColor = texture2D(texture, texCoord);
}
);

string VRAnnotationEngine::gp =
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

void emitChar(in int d, in float p) {
    float f = 0.00833;
    d -= 32;
    if (d >= 0) emitQuad(p, vec4(d*f,d*f+f, 0,1));
}

void emitString(in float str, in float offset) {
    int stri = int(str);
    int c0 = stri;
    int c1 = c0/256;
    int c2 = c1/256;
    c0 = c0%256;
    c1 = c1%256;
    c2 = c2%256;
    if (c0 > 0) emitChar(c0, 3*offset);
    if (c1 > 0) emitChar(c1, 3*offset + 1);
    if (c2 > 0) emitChar(c2, 3*offset + 2);
}

void main() {
    float str = normal[0][0];
    float offset = normal[0][2];
    emitString(str, offset);
}
);








