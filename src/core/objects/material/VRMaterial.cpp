#include "VRMaterial.h"

#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleTexturedMaterial.h>
#include <OpenSG/OSGVariantMaterial.h>
#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGMultiPassMaterial.h>
#include <OpenSG/OSGPrimeMaterial.h>
#include <OpenSG/OSGCompositeMaterial.h>
#include <OpenSG/OSGSwitchMaterial.h>

#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTexGenChunk.h>
#include <OpenSG/OSGLineChunk.h>
#include <OpenSG/OSGPointChunk.h>
#include <OpenSG/OSGPolygonChunk.h>
#include <OpenSG/OSGSimpleSHLChunk.h>
#include <OpenSG/OSGTwoSidedLightingChunk.h>
#include <OpenSG/OSGImage.h>
#include <OpenSG/OSGClipPlaneChunk.h>
#include <OpenSG/OSGStencilChunk.h>
#include "core/objects/VRTransform.h"
#include "core/utils/toString.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/scripting/VRScript.h"
#include "VRVideo.h"
#include "core/tools/VRQRCode.h"
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

struct VRMatData {
    ChunkMaterialRecPtr mat;
    MaterialChunkRecPtr colChunk;
    BlendChunkRecPtr blendChunk;
    TextureEnvChunkRecPtr envChunk;
    TextureObjChunkRecPtr texChunk;
    TexGenChunkRecPtr genChunk;
    LineChunkRecPtr lineChunk;
    PointChunkRecPtr pointChunk;
    PolygonChunkRecPtr polygonChunk;
    TwoSidedLightingChunkRecPtr twoSidedChunk;
    ImageRecPtr texture;
    ShaderProgramChunkRecPtr shaderChunk;
    ClipPlaneChunkRecPtr clipChunk;
    StencilChunkRecPtr stencilChunk;
    ShaderProgramRecPtr vProgram;
    ShaderProgramRecPtr fProgram;
    ShaderProgramRecPtr gProgram;
    VRVideo* video = 0;

    string vertexScript;
    string fragmentScript;
    string geometryScript;

    ~VRMatData() {
        if (video) delete video;
    }

    void reset() {
        mat = ChunkMaterial::create();
        colChunk = MaterialChunk::create();
        colChunk->setBackMaterial(false);
        mat->addChunk(colChunk);
        twoSidedChunk = TwoSidedLightingChunk::create();
        mat->addChunk(twoSidedChunk);
        blendChunk = 0;
        texChunk = 0;
        genChunk = 0;
        envChunk = 0;
        lineChunk = 0;
        pointChunk = 0;
        polygonChunk = 0;
        texture = 0;
        video = 0;
        shaderChunk = 0;
        clipChunk = 0;
        stencilChunk = 0;

        colChunk->setDiffuse( Color4f(1, 1, 1, 1) );
        colChunk->setAmbient( Color4f(0.3, 0.3, 0.3, 1) );
        colChunk->setSpecular( Color4f(1, 1, 1, 1) );
        colChunk->setShininess( 50 );
    }

    VRMatData* copy() {
        VRMatData* m = new VRMatData();
        m->mat = ChunkMaterial::create();

        if (colChunk) { m->colChunk = dynamic_pointer_cast<MaterialChunk>(colChunk->shallowCopy()); m->mat->addChunk(m->colChunk); }
        if (blendChunk) { m->blendChunk = dynamic_pointer_cast<BlendChunk>(blendChunk->shallowCopy()); m->mat->addChunk(m->blendChunk); }
        if (envChunk) { m->envChunk = dynamic_pointer_cast<TextureEnvChunk>(envChunk->shallowCopy()); m->mat->addChunk(m->envChunk); }
        if (texChunk) { m->texChunk = dynamic_pointer_cast<TextureObjChunk>(texChunk->shallowCopy()); m->mat->addChunk(m->texChunk); }
        if (genChunk) { m->genChunk = dynamic_pointer_cast<TexGenChunk>(genChunk->shallowCopy()); m->mat->addChunk(m->genChunk); }
        if (lineChunk) { m->lineChunk = dynamic_pointer_cast<LineChunk>(lineChunk->shallowCopy()); m->mat->addChunk(m->lineChunk); }
        if (pointChunk) { m->pointChunk = dynamic_pointer_cast<PointChunk>(pointChunk->shallowCopy()); m->mat->addChunk(m->pointChunk); }
        if (polygonChunk) { m->polygonChunk = dynamic_pointer_cast<PolygonChunk>(polygonChunk->shallowCopy()); m->mat->addChunk(m->polygonChunk); }
        if (twoSidedChunk) { m->twoSidedChunk = dynamic_pointer_cast<TwoSidedLightingChunk>(twoSidedChunk->shallowCopy()); m->mat->addChunk(m->twoSidedChunk); }
        if (clipChunk) { m->clipChunk = dynamic_pointer_cast<ClipPlaneChunk>(clipChunk->shallowCopy()); m->mat->addChunk(m->clipChunk); }
        if (stencilChunk) { m->stencilChunk = dynamic_pointer_cast<StencilChunk>(stencilChunk->shallowCopy()); m->mat->addChunk(m->stencilChunk); }
        if (shaderChunk) { m->shaderChunk = ShaderProgramChunk::create(); m->mat->addChunk(m->shaderChunk); }

        if (texture) { m->texture = dynamic_pointer_cast<Image>(texture->shallowCopy()); m->texChunk->setImage(m->texture); }
        if (vProgram) { m->vProgram = dynamic_pointer_cast<ShaderProgram>(vProgram->shallowCopy()); m->shaderChunk->addShader(m->vProgram); }
        if (fProgram) { m->fProgram = dynamic_pointer_cast<ShaderProgram>(fProgram->shallowCopy()); m->shaderChunk->addShader(m->fProgram); }
        if (gProgram) { m->gProgram = dynamic_pointer_cast<ShaderProgram>(gProgram->shallowCopy()); m->shaderChunk->addShader(m->gProgram); }
        if (video) ; // TODO

        m->vertexScript = vertexScript;
        m->fragmentScript = fragmentScript;
        m->geometryScript = geometryScript;

        return m;
    }
};

map<string, VRMaterial*> VRMaterial::materials;
map<MaterialRecPtr, VRMaterial*> VRMaterial::materialsByPtr;

VRMaterial::VRMaterial(string name) : VRObject(name) {
    type = "Material";
    addAttachment("material", 0);
    materials[getName()] = this;

    passes = MultiPassMaterial::create();
    addPass();
    activePass = 0;
}

VRMaterial::~VRMaterial() { for (auto m : mats) delete m; }

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
    delete getDefault();
    materials.erase("default");
}

int VRMaterial::getActivePass() { return activePass; }
int VRMaterial::getNPasses() { return passes->getNPasses(); }

int VRMaterial::addPass() {
    activePass = getNPasses();
    VRMatData* md = new VRMatData();
    md->reset();
    passes->addMaterial(md->mat);
    mats.push_back(md);
    return activePass;
}

void VRMaterial::remPass(int i) {
    if (i <= 0 || i >= getNPasses()) return;
    delete mats[i];
    passes->subMaterial(i);
    mats.erase(remove(mats.begin(), mats.end(), mats[i]), mats.end());
    if (activePass == i) activePass = 0;
}

void VRMaterial::setActivePass(int i) {
    if (i < 0 || i >= getNPasses()) return;
    activePass = i;
}

void VRMaterial::setStencilBuffer(bool clear, float value, float mask, int func, int opFail, int opZFail, int opPass) {
    auto md = mats[activePass];
    if (md->stencilChunk == 0) { md->stencilChunk = StencilChunk::create(); md->mat->addChunk(md->stencilChunk); }

    md->stencilChunk->setClearBuffer(clear);
    md->stencilChunk->setStencilFunc(func);
    md->stencilChunk->setStencilValue(value);
    md->stencilChunk->setStencilMask(mask);
    md->stencilChunk->setStencilOpFail(opFail);
    md->stencilChunk->setStencilOpZFail(opZFail);
    md->stencilChunk->setStencilOpZPass(opPass);
}

void VRMaterial::clearExtraPasses() { for (int i=1; i<getNPasses(); i++) remPass(i); }
void VRMaterial::appendPasses(VRMaterial* mat) {
    for (int i=0; i<mat->getNPasses(); i++) {
        VRMatData* md = mat->mats[i]->copy();
        passes->addMaterial(md->mat);
        mats.push_back(md);
    }
}

void VRMaterial::prependPasses(VRMaterial* mat) {
    vector<VRMatData*> pses;
    for (int i=0; i<mat->getNPasses(); i++) pses.push_back( mat->mats[i]->copy() );
    for (int i=0; i<getNPasses(); i++) pses.push_back(mats[i]);

    passes->clearMaterials();
    mats.clear();

    for (auto md : pses) {
        passes->addMaterial(md->mat);
        mats.push_back(md);
    }
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
    cout << "Warning: VRMaterial::copy not implemented!\n";
    // TODO: copy all the stuff
    return mat;
}

void VRMaterial::setLineWidth(int w) {
    auto md = mats[activePass];
    if (md->lineChunk == 0) { md->lineChunk = LineChunk::create(); md->mat->addChunk(md->lineChunk); }
    md->lineChunk->setWidth(w);
    md->lineChunk->setSmooth(true);
}

void VRMaterial::setPointSize(int s) {
    auto md = mats[activePass];
    if (md->pointChunk == 0) { md->pointChunk = PointChunk::create(); md->mat->addChunk(md->pointChunk); }
    md->pointChunk->setSize(s);
    md->pointChunk->setSmooth(true);
}

void VRMaterial::saveContent(xmlpp::Element* e) {
    VRObject::saveContent(e);

    e->set_attribute("diffuse", toString(getDiffuse()));
    e->set_attribute("specular", toString(getSpecular()));
    e->set_attribute("ambient", toString(getAmbient()));
}

void VRMaterial::loadContent(xmlpp::Element* e) {
    VRObject::loadContent(e);

    e->get_attribute("sourcetype")->get_value();
}

void VRMaterial::setMaterial(MaterialRecPtr m) {
    if ( dynamic_pointer_cast<MultiPassMaterial>(m) ) {
        MultiPassMaterialRecPtr mm = dynamic_pointer_cast<MultiPassMaterial>(m);
        for (unsigned int i=0; i<mm->getNPasses(); i++) {
            if (i > 0) addPass();
            setMaterial(mm->getMaterials(i));
        }
        setActivePass(0);
        return;
    }

    if ( isSMat(m) ) {
        SimpleMaterialRecPtr sm = dynamic_pointer_cast<SimpleMaterial>(m);
        setDiffuse(sm->getDiffuse());
        setAmbient(sm->getAmbient());
        setSpecular(sm->getSpecular());

        if ( isSTMat(m) ) {
            SimpleTexturedMaterialRecPtr stm = dynamic_pointer_cast<SimpleTexturedMaterial>(m);
            setTexture(stm->getImage(), true);
        }

        return;
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

        auto md = mats[activePass];
        if (mc) mc->setBackMaterial(false);
        if (mc) { if (md->colChunk) md->mat->subChunk(md->colChunk);   md->colChunk = mc;   md->mat->addChunk(mc); }
        if (bc) { if (md->blendChunk) md->mat->subChunk(md->blendChunk); md->blendChunk = bc; md->mat->addChunk(bc); }
        if (ec) { if (md->envChunk) md->mat->subChunk(md->envChunk);   md->envChunk = ec;   md->mat->addChunk(ec); }
        if (tc) { if (md->texChunk) md->mat->subChunk(md->texChunk);   md->texChunk = tc;   md->mat->addChunk(tc); }
        return;
    }

    cout << "Warning: unhandled material type\n";
    if (dynamic_pointer_cast<Material>(m)) cout << " Material" << endl;
    if (dynamic_pointer_cast<PrimeMaterial>(m)) cout << "  PrimeMaterial" << endl;
    if (dynamic_pointer_cast<SimpleMaterial>(m)) cout << "   SimpleMaterial" << endl;
    if (dynamic_pointer_cast<SimpleTexturedMaterial>(m)) cout << "   SimpleTexturedMaterial" << endl;
    if (dynamic_pointer_cast<VariantMaterial>(m)) cout << "   VariantMaterial" << endl;
    if (dynamic_pointer_cast<ChunkMaterial>(m)) cout << "  ChunkMaterial" << endl;
    if (dynamic_pointer_cast<MultiPassMaterial>(m)) cout << "   MultiPassMaterial" << endl;
    if (dynamic_pointer_cast<CompositeMaterial>(m)) cout << "   CompositeMaterial" << endl;
    if (dynamic_pointer_cast<SwitchMaterial>(m)) cout << "   SwitchMaterial" << endl;
}

MaterialRecPtr VRMaterial::getMaterial() { return passes; }

void VRMaterial::setTextureParams(int min, int mag, int envMode, int wrapS, int wrapT) {
    auto md = mats[activePass];
    if (md->texChunk == 0) { md->texChunk = TextureObjChunk::create(); md->mat->addChunk(md->texChunk); }
    if (md->envChunk == 0) { md->envChunk = TextureEnvChunk::create(); md->mat->addChunk(md->envChunk); }

    md->texChunk->setMinFilter (min);
    md->texChunk->setMagFilter (mag);
    md->envChunk->setEnvMode (envMode);
    md->texChunk->setWrapS (wrapS);
    md->texChunk->setWrapT (wrapT);
}

/** Load a texture && apply it to the mesh as new material **/
void VRMaterial::setTexture(string img_path, bool alpha) { // TODO: improve with texture map
    auto md = mats[activePass];
    if (md->texture == 0) md->texture = Image::create();
    md->texture->read(img_path.c_str());
    setTexture(md->texture, alpha);
}

void VRMaterial::setTexture(ImageRecPtr img, bool alpha) {
    if (img == 0) return;

    auto md = mats[activePass];
    if (md->texChunk == 0) { md->texChunk = TextureObjChunk::create(); md->mat->addChunk(md->texChunk); }
    if (md->envChunk == 0) { md->envChunk = TextureEnvChunk::create(); md->mat->addChunk(md->envChunk); }

    md->texture = img;
    md->texChunk->setImage(img);
    md->envChunk->setEnvMode(GL_MODULATE);
    if (alpha && img->hasAlphaChannel() && md->blendChunk == 0) {
        md->blendChunk = BlendChunk::create();
        md->mat->addChunk(md->blendChunk);
    }

    if (alpha && img->hasAlphaChannel()) {
        md->blendChunk->setSrcFactor  ( GL_SRC_ALPHA           );
        md->blendChunk->setDestFactor ( GL_ONE_MINUS_SRC_ALPHA );
    }
}

void VRMaterial::setTextureType(string type) {
    auto md = mats[activePass];
    if (type == "Normal") {
        if (md->genChunk == 0) return;
        md->mat->subChunk(md->genChunk);
        md->genChunk = 0;
        return;
    }

    if (type == "SphereEnv") {
        if (md->genChunk == 0) { md->genChunk = TexGenChunk::create(); md->mat->addChunk(md->genChunk); }
        md->genChunk->setGenFuncS(GL_SPHERE_MAP);
        md->genChunk->setGenFuncT(GL_SPHERE_MAP);
    }
}

void VRMaterial::setQRCode(string s, Vec3f fg, Vec3f bg, int offset) {
    createQRCode(s, this, fg, bg, offset);
    auto md = mats[activePass];
    md->texChunk->setMagFilter (GL_NEAREST);
    md->texChunk->setMinFilter (GL_NEAREST_MIPMAP_NEAREST);
}

void VRMaterial::setZOffset(float factor, float bias) {
    auto md = mats[activePass];
    if (md->polygonChunk == 0) { md->polygonChunk = PolygonChunk::create(); md->mat->addChunk(md->polygonChunk); }
    md->polygonChunk->setOffsetFactor(factor);
    md->polygonChunk->setOffsetBias(bias);
    md->polygonChunk->setOffsetFill(true);
}

void VRMaterial::setSortKey(int key) {
    auto md = mats[activePass];
    //if (md->polygonChunk == 0) { md->polygonChunk = PolygonChunk::create(); md->mat->addChunk(md->polygonChunk); }
    md->mat->setSortKey(key);
}

void VRMaterial::setFrontBackModes(int front, int back) {
    auto md = mats[activePass];
    if (md->polygonChunk == 0) { md->polygonChunk = PolygonChunk::create(); md->mat->addChunk(md->polygonChunk); }

    md->polygonChunk->setCullFace(GL_NONE);

    if (front == GL_NONE) {
        md->polygonChunk->setFrontMode(GL_FILL);
        md->polygonChunk->setCullFace(GL_FRONT);
    } else md->polygonChunk->setFrontMode(front);

    if (back == GL_NONE) {
        md->polygonChunk->setBackMode(GL_FILL);
        md->polygonChunk->setCullFace(GL_BACK);
    } else md->polygonChunk->setBackMode(back);
}

void VRMaterial::setClipPlane(bool active, Vec4f equation, VRTransform* beacon) {
    for (int i=0; i<getNPasses(); i++) {
        auto md = mats[i];
        if (md->clipChunk == 0) { md->clipChunk = ClipPlaneChunk::create(); md->mat->addChunk(md->clipChunk); }

        md->clipChunk->setEquation(equation);
        md->clipChunk->setEnable  (active);
        if (beacon) md->clipChunk->setBeacon(beacon->getNode());
    }
}

void VRMaterial::setWireFrame(bool b) {
    if (b) setFrontBackModes(GL_LINE, GL_LINE);
    else setFrontBackModes(GL_FILL, GL_FILL);
}

void VRMaterial::setVideo(string vid_path) {
    auto md = mats[activePass];
    if (md->video == 0) md->video = new VRVideo(this);
    md->video->open(vid_path);
}

VRVideo* VRMaterial::getVideo() { return mats[activePass]->video; }

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

            if (smat || stmat)  {
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

void VRMaterial::setTransparency(float c) {
    auto md = mats[activePass];
    md->colChunk->setDiffuse( toColor4f(getDiffuse(), c) );

    if (md->blendChunk == 0) {
        md->blendChunk = BlendChunk::create();
        md->mat->addChunk(md->blendChunk);
        md->blendChunk->setSrcFactor  ( GL_SRC_ALPHA           );
        md->blendChunk->setDestFactor ( GL_ONE_MINUS_SRC_ALPHA );
    }
}

void VRMaterial::setDiffuse(Color3f c) { mats[activePass]->colChunk->setDiffuse( toColor4f(c, getTransparency()) );}
void VRMaterial::setSpecular(Color3f c) { mats[activePass]->colChunk->setSpecular(toColor4f(c)); }
void VRMaterial::setAmbient(Color3f c) { mats[activePass]->colChunk->setAmbient(toColor4f(c)); }
void VRMaterial::setEmission(Color3f c) { mats[activePass]->colChunk->setEmission(toColor4f(c)); }
void VRMaterial::setShininess(float c) { mats[activePass]->colChunk->setShininess(c); }
void VRMaterial::setLit(bool b) { mats[activePass]->colChunk->setLit(b); }

Color3f VRMaterial::getDiffuse() { return toColor3f( mats[activePass]->colChunk->getDiffuse() ); }
Color3f VRMaterial::getSpecular() { return toColor3f( mats[activePass]->colChunk->getSpecular() ); }
Color3f VRMaterial::getAmbient() { return toColor3f( mats[activePass]->colChunk->getAmbient() ); }
Color3f VRMaterial::getEmission() { return toColor3f( mats[activePass]->colChunk->getEmission() ); }
float VRMaterial::getShininess() { return mats[activePass]->colChunk->getShininess(); }
float VRMaterial::getTransparency() { return mats[activePass]->colChunk->getDiffuse()[3]; }
bool VRMaterial::isLit() { return mats[activePass]->colChunk->getLit(); }

ImageRecPtr VRMaterial::getTexture() { return mats[activePass]->texture; }

void VRMaterial::setTexture(char* data, int format, Vec3i dims, bool isfloat) {
    cout << " setTexture " << format << " " << dims << " " << isfloat << endl;
    ImageRecPtr img = Image::create();

    int pf = Image::OSG_RGB_PF;
    if (format == 4) pf = Image::OSG_RGBA_PF;

    int f = Image::OSG_UINT8_IMAGEDATA;
    if (isfloat) f = Image::OSG_FLOAT32_IMAGEDATA;
    img->set( pf, dims[0], dims[1], dims[2], 1, 1, 0, (const UInt8*)data, f);
    if (format == 4) setTexture(img, true);
    if (format == 3) setTexture(img, false);
}

void VRMaterial::initShaderChunk() {
    auto md = mats[activePass];
    if (md->shaderChunk != 0) return;
    md->shaderChunk = ShaderProgramChunk::create();
    md->mat->addChunk(md->shaderChunk);

    md->vProgram = ShaderProgram::createVertexShader  ();
    md->fProgram = ShaderProgram::createFragmentShader();
    md->gProgram = ShaderProgram::createGeometryShader();
    md->shaderChunk->addShader(md->vProgram);
    md->shaderChunk->addShader(md->fProgram);
    md->shaderChunk->addShader(md->gProgram);

    md->vProgram->createDefaulAttribMapping();
    md->vProgram->addOSGVariable("OSGViewportSize");
}

ShaderProgramRecPtr VRMaterial::getShaderProgram() { return mats[activePass]->vProgram; }

void VRMaterial::setVertexShader(string s) {
    initShaderChunk();
    mats[activePass]->vProgram->setProgram(s.c_str());
}

void VRMaterial::setFragmentShader(string s) {
    initShaderChunk();
    mats[activePass]->fProgram->setProgram(s.c_str());
}

void VRMaterial::setGeometryShader(string s) {
    initShaderChunk();
    mats[activePass]->gProgram->setProgram(s.c_str());
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

    auto md = mats[activePass];
    md->texChunk->setMagFilter(Mag);
    md->texChunk->setMinFilter(Min);
}

void VRMaterial::setVertexScript(string script) {
    mats[activePass]->vertexScript = script;
    VRScript* scr = VRSceneManager::getCurrent()->getScript(script);
    if (scr) setVertexShader(scr->getCore());
}

void VRMaterial::setFragmentScript(string script) {
    mats[activePass]->fragmentScript = script;
    VRScript* scr = VRSceneManager::getCurrent()->getScript(script);
    if (scr) setFragmentShader(scr->getCore());
}

void VRMaterial::setGeometryScript(string script) {
    mats[activePass]->geometryScript = script;
    VRScript* scr = VRSceneManager::getCurrent()->getScript(script);
    if (scr) setGeometryShader(scr->getCore());
}

string VRMaterial::getVertexScript() { return mats[activePass]->vertexScript; }
string VRMaterial::getFragmentScript() { return mats[activePass]->fragmentScript; }
string VRMaterial::getGeometryScript() { return mats[activePass]->geometryScript; }

OSG_END_NAMESPACE;
