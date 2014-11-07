#include "VRMaterial.h"
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleTexturedMaterial.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGLineChunk.h>
#include <OpenSG/OSGPointChunk.h>
#include <OpenSG/OSGPolygonChunk.h>
#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGSimpleSHLChunk.h>
#include <OpenSG/OSGImage.h>
#include "core/utils/toString.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/scripting/VRScript.h"
#include "VRVideo.h"
#include "core/tools/VRQRCode.h"
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

map<string, VRMaterial*> VRMaterial::materials;
map<MaterialRecPtr, VRMaterial*> VRMaterial::materialsByPtr;

VRMaterial::VRMaterial(string name) : VRObject(name) {
    type = "Material";
    resetDefault();
    addAttachment("material", 0);
    materials[getName()] = this; // name is always unique ;)
}

VRMaterial::~VRMaterial() {
    if (video) delete video;
}

void VRMaterial::clearAll() {
    for (auto m : materials) delete m.second;
    materials.clear();
    materialsByPtr.clear();
}

VRMaterial* VRMaterial::getDefault() {
    if (materials.count("default") == 1) return materials["default"];
    return new VRMaterial("default");
}

void VRMaterial::resetDefault() {
    mat = ChunkMaterial::create();
    colChunk = MaterialChunk::create();
    mat->addChunk(colChunk);
    blendChunk = 0;
    texChunk = 0;
    envChunk = 0;
    lineChunk = 0;
    pointChunk = 0;
    polygonChunk = 0;
    texture = 0;
    video = 0;
    shaderChunk = 0;

    setDiffuse  (Color3f(.9f,.9f,.8f));
    setAmbient  (Color3f(0.3f,0.3f,0.3f));
    setSpecular (Color3f(1.f,1.f,1.f));
    setShininess(50.f);
}

VRMaterial* VRMaterial::get(MaterialRecPtr mat) {
    if (materialsByPtr.count(mat) == 0) {
        materialsByPtr[mat] = new VRMaterial("mat");
        materialsByPtr[mat]->setMaterial(mat);
    }

    return materialsByPtr[mat];
}

VRMaterial* VRMaterial::get(string s) {
    if (materials.count(s) == 0) materials[s] = new VRMaterial(s);
    return materials[s];
}

VRObject* VRMaterial::copy(vector<VRObject*> children) {
    VRMaterial* mat = new VRMaterial(getBaseName());
    mat->texture = texture;
    mat->mat = this->mat;
    return mat;
}

void VRMaterial::setLineWidth(int w) {
    if (lineChunk == 0) { lineChunk = LineChunk::create(); mat->addChunk(lineChunk); }
    lineChunk->setWidth(w);
}

void VRMaterial::setPointSize(int s) {
    if (pointChunk == 0) { pointChunk = PointChunk::create(); mat->addChunk(pointChunk); }
    pointChunk->setSize(s);
}

void VRMaterial::saveContent(xmlpp::Element* e) {
    VRObject::saveContent(e);

    e->set_attribute("sourcetype", toString(getDiffuse()));
    e->set_attribute("sourcetype", toString(getSpecular()));
    e->set_attribute("sourcetype", toString(getDiffuse()));
}

void VRMaterial::loadContent(xmlpp::Element* e) {
    VRObject::loadContent(e);

    e->get_attribute("sourcetype")->get_value();
}

void VRMaterial::setMaterial(MaterialRecPtr m) {
    if ( isSMat(m) ) {
        SimpleMaterialRecPtr sm = dynamic_pointer_cast<SimpleMaterial>(m);
        setDiffuse(sm->getDiffuse());
        setAmbient(sm->getAmbient());
        setSpecular(sm->getSpecular());
    }
    if ( isSTMat(m) ) {
        SimpleTexturedMaterialRecPtr stm = dynamic_pointer_cast<SimpleTexturedMaterial>(m);
        setDiffuse(stm->getDiffuse());
        setAmbient(stm->getAmbient());
        setSpecular(stm->getSpecular());
        setTexture(stm->getImage());
    }
    if ( isCMat(m) ) {
        MaterialChunkRecPtr mc = 0;
        BlendChunkRecPtr bc = 0;
        TextureEnvChunkRecPtr ec = 0;
        TextureObjChunkRecPtr tc = 0;

        ChunkMaterialRecPtr cmat = dynamic_pointer_cast<ChunkMaterial>(m);
        for (uint i=0; i<cmat->getMFChunks()->size(); i++) {
            StateChunkRecPtr chunk = cmat->getChunk(i);
            if (mc == 0) mc = dynamic_pointer_cast<MaterialChunk>(chunk);
            if (bc == 0) bc = dynamic_pointer_cast<BlendChunk>(chunk);
            if (ec == 0) ec = dynamic_pointer_cast<TextureEnvChunk>(chunk);
            if (tc == 0) tc = dynamic_pointer_cast<TextureObjChunk>(chunk);
        }

        if (mc) { mat->subChunk(colChunk); colChunk = mc; mat->addChunk(colChunk); }
        if (bc) { mat->subChunk(blendChunk); blendChunk = bc; mat->addChunk(blendChunk); }
        if (ec) { mat->subChunk(envChunk); envChunk = ec; mat->addChunk(envChunk); }
        if (tc) { mat->subChunk(texChunk); texChunk = tc; mat->addChunk(texChunk); }
    }
}

MaterialRecPtr VRMaterial::getMaterial() { return mat; }

/** Load a texture and apply it to the mesh as new material **/
void VRMaterial::setTexture(string img_path, bool alpha) { // TODO: improve with texture map
    if (texture == 0) texture = Image::create();
    VRScene* scene = VRSceneManager::get()->getActiveScene();
    img_path = scene->getWorkdir()+"/"+img_path;
    texture->read(img_path.c_str());
    setTexture(texture, alpha);
}

void VRMaterial::setTexture(ImageRecPtr img, bool alpha) {
    if (texChunk == 0) { texChunk = TextureObjChunk::create(); mat->addChunk(texChunk); }
    if (envChunk == 0) { envChunk = TextureEnvChunk::create(); mat->addChunk(envChunk); }

    texture = img;
    texChunk->setImage(img);
    if (alpha and img->hasAlphaChannel() and blendChunk == 0) {
        blendChunk = BlendChunk::create();
        mat->addChunk(blendChunk);
    }

    if (alpha and img->hasAlphaChannel()) {
        envChunk->setEnvMode   (GL_MODULATE);
        blendChunk->setSrcFactor  ( GL_SRC_ALPHA           );
        blendChunk->setDestFactor ( GL_ONE_MINUS_SRC_ALPHA );
    }
}

void VRMaterial::setQRCode(string s, Vec3f fg, Vec3f bg, int offset) {
    createQRCode(s, this, fg, bg, offset);
    texChunk->setMagFilter (GL_NEAREST);
    texChunk->setMinFilter (GL_NEAREST_MIPMAP_NEAREST);
}

void VRMaterial::setWireFrame(bool b) {
    if (polygonChunk == 0) { polygonChunk = PolygonChunk::create(); mat->addChunk(polygonChunk); }
    if (b) {
        polygonChunk->setFrontMode(GL_LINE);
        polygonChunk->setBackMode(GL_LINE);
    } else {
        polygonChunk->setFrontMode(GL_FILL);
        polygonChunk->setBackMode(GL_FILL);
    }
}

void VRMaterial::setVideo(string vid_path) {
    if (video == 0) video = new VRVideo(this);
    video->open(vid_path);
}

VRVideo* VRMaterial::getVideo() { return video; }

void VRMaterial::toggleMaterial(string mat1, string mat2, bool b){
    if (b) setTexture(mat1);
    else setTexture(mat2);
}

bool VRMaterial::isCMat(MaterialUnrecPtr matPtr) { return (dynamic_pointer_cast<ChunkMaterial>(matPtr)); }
bool VRMaterial::isSMat(MaterialUnrecPtr matPtr) { return (dynamic_pointer_cast<SimpleMaterial>(matPtr)); }
bool VRMaterial::isSTMat(MaterialUnrecPtr matPtr) { return (dynamic_pointer_cast<SimpleTexturedMaterial>(matPtr)); }

// to access the protected member of materials
class MAC : private SimpleTexturedMaterial {
    public:

        static MaterialChunkRecPtr getMatChunk(Material* matPtr) {
            if (matPtr == 0) return 0;

            MaterialChunkRecPtr mchunk = 0;
            MaterialRecPtr mat = MaterialRecPtr(matPtr);

            SimpleMaterialRecPtr smat = dynamic_pointer_cast<SimpleMaterial>(mat);
            SimpleTexturedMaterialRecPtr stmat = dynamic_pointer_cast<SimpleTexturedMaterial>(mat);
            ChunkMaterialRecPtr cmat = dynamic_pointer_cast<ChunkMaterial>(mat);

            if (smat or stmat)  {
                MAC* macc = (MAC*)matPtr;
                MaterialChunkRecPtr mchunk = macc->_materialChunk;
                if (mchunk) return mchunk;
            }

            if (cmat) {
                for (uint i=0; i<cmat->getMFChunks()->size(); i++) {
                    StateChunkRecPtr chunk = cmat->getChunk(i);
                    mchunk = dynamic_pointer_cast<MaterialChunk>(chunk);
                    if (mchunk) return mchunk;
                }
            }

            return mchunk;
        }

        static BlendChunkRecPtr getBlendChunk(Material* matPtr) {
            MaterialRecPtr mat = MaterialRecPtr(matPtr);
            ChunkMaterialRecPtr cmat = dynamic_pointer_cast<ChunkMaterial>(mat);
            BlendChunkRecPtr bchunk = 0;
            if (cmat) {
                for (uint i=0; i<cmat->getMFChunks()->size(); i++) {
                    StateChunkRecPtr chunk = cmat->getChunk(i);
                    bchunk = dynamic_pointer_cast<BlendChunk>(chunk);
                    if (bchunk) return bchunk;
                }
            }
            return bchunk;
        }
};

Color4f toColor4f(Color3f c, float t) { return Color4f(c[0], c[1], c[2], t); }
Color3f toColor3f(Color4f c) { return Color3f(c[0], c[1], c[2]); }

void VRMaterial::setDiffuse(Color3f c) { colChunk->setDiffuse( toColor4f(c, getTransparency()) ); }
void VRMaterial::setTransparency(float c) { colChunk->setDiffuse( toColor4f(getDiffuse(), c) ); }
void VRMaterial::setSpecular(Color3f c) { colChunk->setSpecular(toColor4f(c)); }
void VRMaterial::setAmbient(Color3f c) { colChunk->setAmbient(toColor4f(c)); }
void VRMaterial::setEmission(Color3f c) { colChunk->setEmission(toColor4f(c)); }
void VRMaterial::setShininess(float c) { colChunk->setShininess(c); }
void VRMaterial::setLit(bool b) { colChunk->setLit(b); }

Color3f VRMaterial::getDiffuse() { return toColor3f( colChunk->getDiffuse() ); }
Color3f VRMaterial::getSpecular() { return toColor3f( colChunk->getSpecular() ); }
Color3f VRMaterial::getAmbient() { return toColor3f( colChunk->getAmbient() ); }
Color3f VRMaterial::getEmission() { return toColor3f( colChunk->getEmission() ); }
float VRMaterial::getShininess() { return colChunk->getShininess(); }
float VRMaterial::getTransparency() { return colChunk->getDiffuse()[3]; }
bool VRMaterial::isLit() { return colChunk->getLit(); }

ImageRecPtr VRMaterial::getTexture() { return texture; }

void VRMaterial::setVertexShader(string s) {
    if (shaderChunk == 0) { shaderChunk = SimpleSHLChunk::create(); mat->addChunk(shaderChunk); }
    shaderChunk->setVertexProgram(s.c_str());
    shaderChunk->addOSGVariable("OSGViewportSize");
}

void VRMaterial::setFragmentShader(string s) {
    if (shaderChunk == 0) { shaderChunk = SimpleSHLChunk::create(); mat->addChunk(shaderChunk); }
    shaderChunk->setFragmentProgram(s.c_str());
}

void VRMaterial::setGeometryShader(string s) {
    if (shaderChunk == 0) { shaderChunk = SimpleSHLChunk::create(); mat->addChunk(shaderChunk); }
    shaderChunk->setGeometryProgram(s.c_str());
}

string readFile(string path) {
    ifstream file(path.c_str());
    string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return str;
}

void VRMaterial::readVertexShader(string s) { setVertexShader(readFile(s)); }
void VRMaterial::readFragmentShader(string s) { setFragmentShader(readFile(s)); }
void VRMaterial::readGeometryShader(string s) { setGeometryShader(readFile(s)); }

string VRMaterial::getVertexShader() { return ""; } // TODO
string VRMaterial::getFragmentShader() { return ""; }
string VRMaterial::getGeometryShader() { return ""; }

void VRMaterial::setMagMinFilter(string mag, string min) {
    GLenum Mag, Min;

    if (mag == "GL_NEAREST") Mag = GL_NEAREST;
    if (mag == "GL_LINEAR") Mag = GL_LINEAR;
    if (min == "GL_NEAREST") Min = GL_NEAREST;
    if (min == "GL_LINEAR") Min = GL_LINEAR;
    if (min == "GL_NEAREST_MIPMAP_NEAREST") Min = GL_NEAREST_MIPMAP_NEAREST;
    if (min == "GL_LINEAR_MIPMAP_NEAREST") Min = GL_LINEAR_MIPMAP_NEAREST;
    if (min == "GL_NEAREST_MIPMAP_LINEAR") Min = GL_NEAREST_MIPMAP_LINEAR;
    if (min == "GL_LINEAR_MIPMAP_LINEAR") Min = GL_LINEAR_MIPMAP_LINEAR;

    texChunk->setMagFilter(Mag);
    texChunk->setMinFilter(Min);
}

void VRMaterial::setVertexProgram(string script) {
    vertexScript = script;
    VRScript* scr = VRSceneManager::get()->getActiveScene()->getScript(script);
    if (scr) setVertexShader(scr->getCore());
}

void VRMaterial::setFragmentProgram(string script) {
    fragmentScript = script;
    VRScript* scr = VRSceneManager::get()->getActiveScene()->getScript(script);
    if (scr) setFragmentShader(scr->getCore());
}

void VRMaterial::setGeometryProgram(string script) {
    geometryScript = script;
    VRScript* scr = VRSceneManager::get()->getActiveScene()->getScript(script);
    if (scr) setGeometryShader(scr->getCore());
}

OSG_END_NAMESPACE;
