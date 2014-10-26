#include "VRNumberingEngine.h"

#include "core/objects/material/VRMaterial.h"
#include "core/tools/VRText.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>

using namespace OSG;
using namespace std;

VRNumberingEngine::VRNumberingEngine() : VRGeometry("NumbEng") {
    OSG::GeoPnt3fPropertyRecPtr pos = OSG::GeoPnt3fProperty::create();
    OSG::GeoVec3fPropertyRecPtr norms = OSG::GeoVec3fProperty::create();
    OSG::GeoUInt32PropertyRefPtr inds = OSG::GeoUInt32Property::create();

    this->pos = pos;
    this->norms = norms;

    setType(GL_POINTS);
    setPositions(pos);
    setNormals(norms);
    setIndices(inds);

    mat = new VRMaterial("NumbEngMat");
    mat->setVertexShader(vp);
    mat->setFragmentShader(fp);
    mat->setGeometryShader(gp);
    updateTexture();
    setMaterial(mat);

    group g;
    groups.push_back(g);
}

bool VRNumberingEngine::checkUIn(int grp) {
    if (grp < 0 or grp > (int)groups.size()) return true;
    return false;
}

bool VRNumberingEngine::checkUIn(int i, int grp) {
    if (grp < 0 or grp > (int)groups.size()) return true;
    if (i < 0 or i > (int)pos->size()) return true;
    return false;
}

void VRNumberingEngine::add(Vec3f p, int N, float f, int d, int grp) {
    if (checkUIn(grp)) return;

    int s = pos->size();
    mesh->getLengths()->setValue(N+s, 0);

    group g = groups[grp];
    for (int i=0; i<N; i++) {
        pos->addValue(p);
        norms->addValue(Vec3f(0,grp,0));
        mesh->getIndices()->addValue(i+s);
    }
}

void VRNumberingEngine::set(int i, Vec3f p, float f, int d, int grp) {
    if (checkUIn(i,grp)) return;
    pos->setValue(p, i);
    float f1 = floor(f);
    float f2 = f-f1;
    string sf2 = toString(f2);
    if (sf2.size() > 2) sf2 = sf2.substr(2,sf2.size()-1);
    f2 = toFloat(sf2);
    norms->setValue(Vec3f(f1,grp,f2), i);
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

void VRNumberingEngine::updateTexture() {
    string txt = "0123456789.";
    for (auto g : groups) {
        txt += "\n"+g.pre+"\n"+g.post;
    }

    ImageRecPtr img = VRText::get()->create(txt, "MONO 20", 20, Color4f(0,0,0,255), Color4f(0,0,0,0) );
    mat->setTexture(img);
}

string VRNumberingEngine::vp = ""
"#version 120\n"
"varying mat4 model;\n"
"varying vec3 normal;\n"

"void main( void ) {\n"
"    model = gl_ModelViewProjectionMatrix;\n"
"    gl_Position = model*gl_Vertex;\n"
"    normal = gl_Normal;\n"
"}";

string VRNumberingEngine::fp = ""
"#version 120\n"
"uniform sampler2D texture;\n"
"in vec2 texCoord;\n"

"void main( void ) {\n"
"  gl_FragColor = texture2D(texture, texCoord);\n"
"}";

string VRNumberingEngine::gp = ""
"#version 150\n"
"layout (points) in;\n"
"layout (triangle_strip, max_vertices=60) out;\n"

"in mat4 model[];\n"
"in vec3 normal[];\n"
"out vec2 texCoord;\n"

"void emitVertex(in vec4 p, in vec2 tc) {\n"
"   gl_Position = p;\n"
"   texCoord = tc;\n"
"   EmitVertex();\n"
"}\n"

"void emitQuad(in float offset, in vec4 tc) {\n"
"   float sx = 0.1;\n"
"   float sy = 0.2;\n"
"   float ox = 2*sx*offset;\n"
"   vec4 p1 = gl_PositionIn[0]+model[0]*vec4(-sx+ox,-sy,0,0);\n"
"   vec4 p2 = gl_PositionIn[0]+model[0]*vec4(-sx+ox, sy,0,0);\n"
"   vec4 p3 = gl_PositionIn[0]+model[0]*vec4( sx+ox, sy,0,0);\n"
"   vec4 p4 = gl_PositionIn[0]+model[0]*vec4( sx+ox,-sy,0,0);\n"

"   emitVertex(p1, vec2(tc[0], tc[2]));\n"
"   emitVertex(p2, vec2(tc[0], tc[3]));\n"
"   emitVertex(p3, vec2(tc[1], tc[3]));\n"
"   EndPrimitive();\n"
"   emitVertex(p1, vec2(tc[0], tc[2]));\n"
"   emitVertex(p3, vec2(tc[1], tc[3]));\n"
"   emitVertex(p4, vec2(tc[1], tc[2]));\n"
"   EndPrimitive();\n"
"}\n"

"void emitDot(in float p) {\n"
" float f = 0.0727;\n"
" int d = 10;\n"
" emitQuad(p, vec4(d*f,d*f+f, 0,1));\n"
"}\n"

"void emitDigit(in int d, in float p) {\n"
" float f = 0.0727;\n"
" emitQuad(p, vec4(d*f,d*f+f, 0,1));\n"
"}\n"

"void emitNumber(in float n1, in float n2, in int N) {\n"
" int d=0;\n"
" int k1 = int(n1);\n"
" int k2 = int(n2);\n"
" int i=0;\n"
" int first = 1;\n"
" float p = 0;\n"

" while(1) {\n"
"   if (first == 1) {\n"
"     d = k2%10;\n"
"     k2 = int(k2*0.1);\n"
"     p = -i*1.5;\n"
"     emitDigit(d, p);\n"
"   } else {\n"
"     d = k1%10;\n"
"     k1 = int(k1*0.1);\n"
"     p = -i*1.5;\n"
"     emitDigit(d, p);\n"
"   }\n"

"   if (first == 1) {\n"
"     if (k2 == 0 || i == N) {\n"
"       i++;\n"
"       p = -i*1.5;\n"
"       emitDot(p);\n"
"       first = 2;\n"
"       k2 = 0;\n"
"     }\n"
"   }\n"

"   i++;\n"
"   if (k1 == 0 && k2 == 0) return;\n"
"   if (i > 10) return;\n"
" }\n"
"}\n"

"void main() {\n"
"  emitNumber(normal[0][0], normal[0][2], 1);\n"
"}";








