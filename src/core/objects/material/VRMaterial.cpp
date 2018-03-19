#include "VRMaterial.h"
#include "OSGMaterial.h"
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"
#include "core/objects/VRTransform.h"
#include "core/objects/object/OSGCore.h"
#include "core/objects/OSGObject.h"
#include "core/objects/material/VRTexture.h"
#include "core/utils/toString.h"
#include "core/utils/VRUndoInterfaceT.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRWindow.h"
#include "core/scripting/VRScript.h"
#include "VRVideo.h"
#include "core/tools/VRQRCode.h"
#include "core/utils/system/VRSystem.h"

#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGMaterialGroup.h>
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
#include <OpenSG/OSGDepthChunk.h>
#include <OpenSG/OSGMaterialChunk.h>

#include <libxml++/nodes/element.h>
#include <GL/glext.h>
#include <GL/glx.h>

template<> string typeName(const OSG::VRMaterialPtr& t) { return "Material"; }

OSG_BEGIN_NAMESPACE;
using namespace std;

struct VRMatData {
    ChunkMaterialMTRecPtr mat;
    MaterialChunkMTRecPtr colChunk;
    BlendChunkMTRecPtr blendChunk;
    DepthChunkMTRecPtr depthChunk;
    map<int, TextureObjChunkMTRecPtr> texChunks;
    map<int, TextureEnvChunkMTRecPtr> envChunks;
    TexGenChunkMTRecPtr genChunk;
    LineChunkMTRecPtr lineChunk;
    PointChunkMTRecPtr pointChunk;
    PolygonChunkMTRecPtr polygonChunk;
    TwoSidedLightingChunkMTRecPtr twoSidedChunk;
    //VRTexturePtr texture;
    ShaderProgramChunkMTRecPtr shaderChunk;
    ClipPlaneChunkMTRecPtr clipChunk;
    StencilChunkMTRecPtr stencilChunk;
    ShaderProgramMTRecPtr vProgram;
    ShaderProgramMTRecPtr fProgram;
    ShaderProgramMTRecPtr fdProgram;
    ShaderProgramMTRecPtr gProgram;
    ShaderProgramMTRecPtr tcProgram;
    ShaderProgramMTRecPtr teProgram;
    VRVideo* video = 0;
    bool deferred = false;
    bool tmpDeferredShdr = false;

    string vertexScript;
    string fragmentScript;
    string fragmentDScript;
    string geometryScript;
    string tessControlScript;
    string tessEvalScript;

    ~VRMatData() {
        if (video) delete video;
    }

    int getTextureDimension(int unit = 0) {
        if (texChunks.count(unit) == 0) return 0;
        ImageMTRecPtr img = texChunks[unit]->getImage();
        if (img == 0) return 0;
        return img->getDimension();
    }

    void reset() {
        mat = ChunkMaterial::create();
        colChunk = MaterialChunk::create();
        colChunk->setBackMaterial(false);
        mat->addChunk(colChunk);
        twoSidedChunk = TwoSidedLightingChunk::create();
        mat->addChunk(twoSidedChunk);
        blendChunk = 0;
        depthChunk = 0;
        genChunk = 0;
        lineChunk = 0;
        pointChunk = 0;
        polygonChunk = 0;
        //texture = 0;
        video = 0;
        shaderChunk = 0;
        clipChunk = 0;
        stencilChunk = 0;
        deferred = false;
        tmpDeferredShdr = false;

        colChunk->setDiffuse( Color4f(1, 1, 1, 1) );
        colChunk->setAmbient( Color4f(0.3, 0.3, 0.3, 1) );
        colChunk->setSpecular( Color4f(1, 1, 1, 1) );
        colChunk->setShininess( 50 );
    }

    VRMatDataPtr copy() {
        VRMatDataPtr m = VRMatDataPtr( new VRMatData() );
        m->mat = ChunkMaterial::create();

        if (colChunk) { m->colChunk = dynamic_pointer_cast<MaterialChunk>(colChunk->shallowCopy()); m->mat->addChunk(m->colChunk); }
        if (blendChunk) { m->blendChunk = dynamic_pointer_cast<BlendChunk>(blendChunk->shallowCopy()); m->mat->addChunk(m->blendChunk); }
        if (depthChunk) { m->depthChunk = dynamic_pointer_cast<DepthChunk>(depthChunk->shallowCopy()); m->mat->addChunk(m->depthChunk); }
        for (auto t : texChunks) { TextureObjChunkMTRecPtr mt = dynamic_pointer_cast<TextureObjChunk>(t.second->shallowCopy()); m->texChunks[t.first] = mt; m->mat->addChunk(mt, t.first); }
        for (auto t : envChunks) { TextureEnvChunkMTRecPtr mt = dynamic_pointer_cast<TextureEnvChunk>(t.second->shallowCopy()); m->envChunks[t.first] = mt; m->mat->addChunk(mt, t.first); }
        if (genChunk) { m->genChunk = dynamic_pointer_cast<TexGenChunk>(genChunk->shallowCopy()); m->mat->addChunk(m->genChunk); }
        if (lineChunk) { m->lineChunk = dynamic_pointer_cast<LineChunk>(lineChunk->shallowCopy()); m->mat->addChunk(m->lineChunk); }
        if (pointChunk) { m->pointChunk = dynamic_pointer_cast<PointChunk>(pointChunk->shallowCopy()); m->mat->addChunk(m->pointChunk); }
        if (polygonChunk) { m->polygonChunk = dynamic_pointer_cast<PolygonChunk>(polygonChunk->shallowCopy()); m->mat->addChunk(m->polygonChunk); }
        if (twoSidedChunk) { m->twoSidedChunk = dynamic_pointer_cast<TwoSidedLightingChunk>(twoSidedChunk->shallowCopy()); m->mat->addChunk(m->twoSidedChunk); }
        if (clipChunk) { m->clipChunk = dynamic_pointer_cast<ClipPlaneChunk>(clipChunk->shallowCopy()); m->mat->addChunk(m->clipChunk); }
        if (stencilChunk) { m->stencilChunk = dynamic_pointer_cast<StencilChunk>(stencilChunk->shallowCopy()); m->mat->addChunk(m->stencilChunk); }
        if (shaderChunk) { m->shaderChunk = ShaderProgramChunk::create(); m->mat->addChunk(m->shaderChunk); }

        /*if (texture) {
                ImageMTRecPtr img = dynamic_pointer_cast<Image>(texture->getImage()->shallowCopy());
                m->texture = VRTexture::create(img);
        }*/

        if (vProgram) { m->vProgram = dynamic_pointer_cast<ShaderProgram>(vProgram->shallowCopy()); m->shaderChunk->addShader(m->vProgram); }
        if (fProgram) { m->fProgram = dynamic_pointer_cast<ShaderProgram>(fProgram->shallowCopy()); m->shaderChunk->addShader(m->fProgram); }
        if (fdProgram) { m->fdProgram = dynamic_pointer_cast<ShaderProgram>(fdProgram->shallowCopy()); }
        if (gProgram) { m->gProgram = dynamic_pointer_cast<ShaderProgram>(gProgram->shallowCopy()); m->shaderChunk->addShader(m->gProgram); }
        if (tcProgram) { m->tcProgram = dynamic_pointer_cast<ShaderProgram>(tcProgram->shallowCopy()); m->shaderChunk->addShader(m->tcProgram); }
        if (teProgram) { m->teProgram = dynamic_pointer_cast<ShaderProgram>(teProgram->shallowCopy()); m->shaderChunk->addShader(m->teProgram); }
        if (video) ; // TODO

        m->vertexScript = vertexScript;
        m->fragmentScript = fragmentScript;
        m->fragmentDScript = fragmentDScript;
        m->geometryScript = geometryScript;
        m->tessControlScript = tessControlScript;
        m->tessEvalScript = tessEvalScript;
        m->toggleDeferredShader(deferred);

        return m;
    }

    void toggleDeferredShader(bool def, string name = "") {
        if (deferred == def) return;
        deferred = def;
        if (!shaderChunk) return;
        shaderChunk->subFragmentShader(0);
        if (deferred) shaderChunk->addShader(fdProgram);
        else shaderChunk->addShader(fProgram);
    }

    template<typename T>
    void clearChunk(T& c) {
        if (c) { mat->subChunk(c); c = 0; }
    }
};

map<string, VRMaterialWeakPtr> VRMaterial::materials;
map<MaterialMTRecPtr, VRMaterialWeakPtr> VRMaterial::materialsByPtr;

VRMaterial::VRMaterial(string name) : VRObject(name) {
    auto scene = VRScene::getCurrent();
    if (scene) deferred = scene->getDefferedShading();
    addAttachment("material", 0);
    passes = OSGMaterial::create( MultiPassMaterial::create() );
    addPass();
    activePass = 0;

    MaterialGroupMTRecPtr group = MaterialGroup::create();
    group->setMaterial(passes->mat);
    setCore(OSGCore::create(group), "Material");

    //store("diffuse", &diffuse);
    //store("specular", &specular);
    //store("ambient", &ambient);
}

VRMaterial::~VRMaterial() {}

VRMaterialPtr VRMaterial::ptr() { return static_pointer_cast<VRMaterial>( shared_from_this() ); }

VRMaterialPtr VRMaterial::create(string name) {
    auto p = VRMaterialPtr(new VRMaterial(name) );
    materials[p->getName()] = p;
    return p;
}

string VRMaterial::constructShaderVP(VRMatDataPtr data) {
    int texD = data->getTextureDimension();

    string vp;
    vp += "#version 120\n";
    vp += "attribute vec4 osg_Vertex;\n";
    vp += "attribute vec3 osg_Normal;\n";
    //vp += "attribute vec4 osg_Color;\n";
    if (texD == 2) vp += "attribute vec2 osg_MultiTexCoord0;\n";
    if (texD == 3) vp += "attribute vec3 osg_MultiTexCoord0;\n";
    vp += "varying vec4 vertPos;\n";
    vp += "varying vec3 vertNorm;\n";
    vp += "varying vec3 color;\n";
    vp += "void main(void) {\n";
    vp += "  vertPos = gl_ModelViewMatrix * osg_Vertex;\n";
    vp += "  vertNorm = gl_NormalMatrix * osg_Normal;\n";
    if (texD == 2) vp += "  gl_TexCoord[0] = vec4(osg_MultiTexCoord0,0.0,0.0);\n";
    if (texD == 3) vp += "  gl_TexCoord[0] = vec4(osg_MultiTexCoord0,0.0);\n";
    vp += "  color  = gl_Color.rgb;\n";
    vp += "  gl_Position    = gl_ModelViewProjectionMatrix*osg_Vertex;\n";
    vp += "}\n";
    return vp;
}

string VRMaterial::constructShaderFP(VRMatDataPtr data) {
    int texD = data->getTextureDimension();

    string fp;
    fp += "#version 120\n";
    fp += "uniform int isLit;\n";
    fp += "varying vec4 vertPos;\n";
    fp += "varying vec3 vertNorm;\n";
    fp += "varying vec3 color;\n";
    //fp += "float luminance(vec3 col) { return dot(col, vec3(0.3, 0.59, 0.11)); }\n";
    fp += "float luminance(vec3 col) { return 1; }\n";
    if (texD == 2) fp += "uniform sampler2D tex0;\n";
    if (texD == 3) fp += "uniform sampler3D tex0;\n";
    fp += "void main(void) {\n";
    fp += "  vec3 pos = vertPos.xyz / vertPos.w;\n";
    if (texD == 0) fp += "  vec3 diffCol = color;\n";
    if (texD == 2) fp += "  vec3 diffCol = texture2D(tex0, gl_TexCoord[0].xy).rgb;\n";
    if (texD == 3) fp += "  vec3 diffCol = texture3D(tex0, gl_TexCoord[0].xyz).rgb;\n";
    fp += "  float ambVal  = luminance(diffCol);\n";
    fp += "  gl_FragData[0] = vec4(pos, ambVal);\n";
    fp += "  gl_FragData[1] = vec4(normalize(vertNorm), isLit);\n";
    fp += "  gl_FragData[2] = vec4(diffCol, 0);\n";
    fp += "}\n";
    return fp;
}

void VRMaterial::updateDeferredShader() {
    auto m = mats[activePass];
    if (!m->tmpDeferredShdr) return;

    initShaderChunk();
    string s = constructShaderVP(m);
    m->vProgram->setProgram(s.c_str());
    checkShader(GL_VERTEX_SHADER, s, "defferedVS");

    s = constructShaderFP(m);
    m->fdProgram->setProgram(s.c_str());
    checkShader(GL_FRAGMENT_SHADER, s, "defferedFS");

    setShaderParameter("isLit", int(isLit()));
}

void VRMaterial::setDeferred(bool b) {
    deferred = b;
    int a = activePass;
    for (uint i=0; i<mats.size(); i++) {
        setActivePass(i);
        if (b) {
            if (mats[i]->shaderChunk == 0) {
                mats[i]->tmpDeferredShdr = true;
                updateDeferredShader();
            }
        } else if (mats[i]->tmpDeferredShdr) remShaderChunk();
        mats[i]->toggleDeferredShader(b, getName());
    }
    setActivePass(a);
}

void VRMaterial::testFix() {
    auto m = mats[activePass];

    initShaderChunk();
    string s = constructShaderVP(m);
    m->vProgram->setProgram(s.c_str());
    checkShader(GL_VERTEX_SHADER, s, "defferedVS");

    string s = constructShaderFP(m);
    m->fdProgram->setProgram(s.c_str());
    checkShader(GL_FRAGMENT_SHADER, s, "defferedFS");

    setShaderParameter("isLit", int(isLit()));
}

void VRMaterial::clearAll() {
    materials.clear();
    materialsByPtr.clear();
}

vector<VRMaterialPtr> VRMaterial::getAll() {
    vector<VRMaterialPtr> res;
    for (auto wm : materialsByPtr) if (auto m = wm.second.lock()) res.push_back(m);
    return res;
}

VRMaterialPtr VRMaterial::getDefault() {
    if (materials.count("default"))
        if (auto sp = materials["default"].lock()) return sp;
    return VRMaterial::create("default");
}

void VRMaterial::resetDefault() {
    materials.erase("default");
}

int VRMaterial::getActivePass() { return activePass; }
int VRMaterial::getNPasses() { return passes->mat->getNPasses(); }

int VRMaterial::addPass() {
    activePass = getNPasses();
    VRMatDataPtr md = VRMatDataPtr( new VRMatData() );
    md->reset();
    passes->mat->addMaterial(md->mat);
    mats.push_back(md);
    setDeferred(deferred);
    return activePass;
}

void VRMaterial::remPass(int i) {
    if (i <= 0 || i >= getNPasses()) return;
    passes->mat->subMaterial(i);
    mats.erase(remove(mats.begin(), mats.end(), mats[i]), mats.end());
    if (activePass == i) activePass = 0;
}

void VRMaterial::setActivePass(int i) {
    if (i < 0) return;
    while (i >= getNPasses()) addPass();
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
void VRMaterial::appendPasses(VRMaterialPtr mat) {
    for (int i=0; i<mat->getNPasses(); i++) {
        VRMatDataPtr md = mat->mats[i]->copy();
        passes->mat->addMaterial(md->mat);
        mats.push_back(md);
    }
}

void VRMaterial::prependPasses(VRMaterialPtr mat) {
    vector<VRMatDataPtr> pses;
    for (int i=0; i<mat->getNPasses(); i++) pses.push_back( mat->mats[i]->copy() );
    for (int i=0; i<getNPasses(); i++) pses.push_back(mats[i]);

    passes->mat->clearMaterials();
    mats.clear();

    for (auto md : pses) {
        passes->mat->addMaterial(md->mat);
        mats.push_back(md);
    }
}

VRMaterialPtr VRMaterial::get(MaterialMTRecPtr mat) {
    VRMaterialPtr m;
    if (materialsByPtr.count(mat) == 0) {
        m = VRMaterial::create("mat");
        m->setMaterial(mat);
        materialsByPtr[mat] = m;
        return m;
    } else if (materialsByPtr[mat].lock() == 0) {
        m = VRMaterial::create("mat");
        m->setMaterial(mat);
        materialsByPtr[mat] = m;
        return m;
    }

    return materialsByPtr[mat].lock();
}

VRMaterialPtr VRMaterial::get(string s) {
    VRMaterialPtr mat;
    if (materials.count(s) == 0) {
        mat = VRMaterial::create(s);
        materials[s] = mat;
        return mat;
    } else if (materials[s].lock() == 0) {
        mat = VRMaterial::create(s);
        materials[s] = mat;
        return mat;
    }

    return materials[s].lock();
}

VRObjectPtr VRMaterial::copy(vector<VRObjectPtr> children) { // TODO: test it, may not work properly!
    VRMaterialPtr mat = VRMaterial::create(getBaseName());

    for (uint i=0; i<mats.size(); i++) {
        if (i > 0) mat->addPass();
        mat->mats[i] = mats[i]->copy();
    }

    mat->force_transparency = force_transparency;
    mat->deferred = deferred;
    mat->activePass = activePass;
    return mat;
}

void VRMaterial::setLineWidth(int w, bool smooth) {
    auto md = mats[activePass];
    if (md->lineChunk == 0) { md->lineChunk = LineChunk::create(); md->mat->addChunk(md->lineChunk); }
    md->lineChunk->setWidth(w);
    md->lineChunk->setSmooth(smooth);
}

void VRMaterial::setPointSize(int s, bool smooth) {
    auto md = mats[activePass];
    if (md->pointChunk == 0) { md->pointChunk = PointChunk::create(); md->mat->addChunk(md->pointChunk); }
    md->pointChunk->setSize(s);
    md->pointChunk->setSmooth(smooth);
}

void VRMaterial::setMaterial(MaterialMTRecPtr m) {
    if ( dynamic_pointer_cast<MultiPassMaterial>(m) ) {
        MultiPassMaterialMTRecPtr mm = dynamic_pointer_cast<MultiPassMaterial>(m);
        for (unsigned int i=0; i<mm->getNPasses(); i++) {
            if (i > 0) addPass();
            setMaterial(mm->getMaterials(i));
        }
        setActivePass(0);
        return;
    }

    if ( isSMat(m) ) {
        SimpleMaterialMTRecPtr sm = dynamic_pointer_cast<SimpleMaterial>(m);
        setDiffuse(sm->getDiffuse());
        setAmbient(sm->getAmbient());
        setSpecular(sm->getSpecular());

        if ( isSTMat(m) ) {
            SimpleTexturedMaterialMTRecPtr stm = dynamic_pointer_cast<SimpleTexturedMaterial>(m);
            setTexture( VRTexture::create(stm->getImage()), true);
        }
        updateDeferredShader();
        return;
    }

    if ( isCMat(m) ) {
        auto md = mats[activePass];

        ChunkMaterialMTRecPtr cmat = dynamic_pointer_cast<ChunkMaterial>(m);
        for (uint i=0; i<cmat->getMFChunks()->size(); i++) {
            StateChunkMTRecPtr chunk = cmat->getChunk(i);
            int unit = -2; cmat->getChunkSlot(chunk, unit);

            MaterialChunkMTRecPtr mc = dynamic_pointer_cast<MaterialChunk>(chunk);
            BlendChunkMTRecPtr bc = dynamic_pointer_cast<BlendChunk>(chunk);
            TextureEnvChunkMTRecPtr ec = dynamic_pointer_cast<TextureEnvChunk>(chunk);
            TextureObjChunkMTRecPtr tc = dynamic_pointer_cast<TextureObjChunk>(chunk);
            DepthChunkMTRecPtr dc = dynamic_pointer_cast<DepthChunk>(chunk);
            TwoSidedLightingChunkMTRecPtr tsc = dynamic_pointer_cast<TwoSidedLightingChunk>(chunk);

            if (mc) { md->colChunk = mc; mc->setBackMaterial(false); md->mat->addChunk(mc,unit); continue; }
            if (bc) { md->blendChunk = bc; md->mat->addChunk(bc,unit); continue; }
            if (ec) { md->envChunks[unit] = ec; md->mat->addChunk(ec,unit); continue; }
            if (tc) { md->texChunks[unit] = tc; md->mat->addChunk(tc,unit); continue; }
            if (dc) { md->depthChunk = dc; md->mat->addChunk(dc,unit); continue; }
            if (tsc) { md->twoSidedChunk = tsc; md->mat->addChunk(tsc,unit); continue; }

            cout << "isCMat unhandled chunk: " << chunk->getClass()->getName() << endl;
        }
        updateDeferredShader();
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

OSGMaterialPtr VRMaterial::getMaterial() { return passes; }
ChunkMaterialMTRecPtr VRMaterial::getMaterial(int i) { return mats[i]->mat; }

void VRMaterial::setTextureParams(int min, int mag, int envMode, int wrapS, int wrapT, int unit) {
    auto md = mats[activePass];
    if (md->texChunks.count(unit) == 0) { md->texChunks[unit] = TextureObjChunk::create(); md->mat->addChunk(md->texChunks[unit], unit); }
    if (md->envChunks.count(unit) == 0) { md->envChunks[unit] = TextureEnvChunk::create(); md->mat->addChunk(md->envChunks[unit], unit); }

    int Min = min < 0 ? md->texChunks[unit]->getMinFilter() : min;
    int Mag = mag < 0 ? md->texChunks[unit]->getMagFilter() : mag;
    int EnvMode = envMode < 0 ? md->envChunks[unit]->getEnvMode() : envMode;
    int WrapS = wrapS < 0 ? md->texChunks[unit]->getWrapS() : wrapS;
    int WrapT = wrapT < 0 ? md->texChunks[unit]->getWrapT() : wrapT;

    md->texChunks[unit]->setMinFilter (Min);
    md->texChunks[unit]->setMagFilter (Mag);
    md->envChunks[unit]->setEnvMode (EnvMode);
    md->texChunks[unit]->setWrapS (WrapS);
    md->texChunks[unit]->setWrapT (WrapT);
}

void VRMaterial::setMagMinFilter(int mag, int min, int unit) {
    setTextureParams(min, mag, -1, -1, -1, unit);
}

void VRMaterial::setTextureWrapping(int wrapS, int wrapT, int unit) {
    setTextureParams(-1, -1, -1, wrapS, wrapT, unit);
}

void VRMaterial::setTexture(string img_path, bool alpha, int unit) { // TODO: improve with texture map
    if (exists(img_path)) img_path = canonical(img_path);
    /*auto md = mats[activePass];
    if (md->texture == 0) md->texture = VRTexture::create();
    md->texture->getImage()->read(img_path.c_str());
    setTexture(md->texture, alpha, unit);*/
    auto tex = VRTexture::create();
    tex->getImage()->read(img_path.c_str());
    setTexture(tex, alpha, unit);
}

void VRMaterial::setTexture(VRTexturePtr img, bool alpha, int unit) {
    if (img == 0) return;
    if (img->getImage() == 0) return;

    auto md = mats[activePass];
    if (md->texChunks.count(unit) == 0) { md->texChunks[unit] = TextureObjChunk::create(); md->mat->addChunk(md->texChunks[unit], unit); }
    if (md->envChunks.count(unit) == 0) { md->envChunks[unit] = TextureEnvChunk::create(); md->mat->addChunk(md->envChunks[unit], unit); }

    //md->texture = img;
    md->texChunks[unit]->setImage(img->getImage());
    md->envChunks[unit]->setEnvMode(GL_MODULATE);
    if (alpha && img->getImage()->hasAlphaChannel()) enableTransparency(false);

    md->texChunks[unit]->setInternalFormat(img->getInternalFormat());
    updateDeferredShader();
}

void VRMaterial::setTexture(char* data, int format, Vec3i dims, bool isfloat) {
    VRTexturePtr img = VRTexture::create();

    int pf = Image::OSG_RGB_PF;
    if (format == 4) pf = Image::OSG_RGBA_PF;

    int f = Image::OSG_UINT8_IMAGEDATA;
    if (isfloat) f = Image::OSG_FLOAT32_IMAGEDATA;
    img->getImage()->set( pf, dims[0], dims[1], dims[2], 1, 1, 0, (const UInt8*)data, f);
    if (format == 4) setTexture(img, true);
    if (format == 3) setTexture(img, false);
    setShaderParameter<int>("is3DTexture", dims[2] > 1);
}

TextureObjChunkMTRefPtr VRMaterial::getTexChunk(int unit) {
    auto md = mats[activePass];

    if (md->texChunks.count(unit) == 0) {
        md->texChunks[unit] = TextureObjChunk::create();
        md->envChunks[unit] = TextureEnvChunk::create();
        md->mat->addChunk(md->texChunks[unit], unit);
        md->mat->addChunk(md->envChunks[unit], unit);
        md->envChunks[unit]->setEnvMode(GL_MODULATE);
    }

    return md->texChunks[unit];
}

void VRMaterial::setTexture(TextureObjChunkMTRefPtr texChunk, int unit) {
    auto md = mats[activePass];
    md->texChunks[unit] = texChunk;
    md->envChunks[unit] = TextureEnvChunk::create();
    md->envChunks[unit]->setEnvMode(GL_MODULATE);
    md->mat->addChunk(md->texChunks[unit], unit);
    md->mat->addChunk(md->envChunks[unit], unit);
}

void VRMaterial::setTextureAndUnit(VRTexturePtr img, int unit) {
    if (img == 0) return;
    auto texChunk = getTexChunk(unit);
    texChunk->setImage(img->getImage());
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

void VRMaterial::setQRCode(string s, Vec3d fg, Vec3d bg, int offset) {
    createQRCode(s, ptr(), fg, bg, offset);
    setTextureParams(GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, -1, -1, -1);
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
    //if (md->polygonChunk == 0) { md->polygonChunk = polygonChunk::create(); md->mat->addChunk(md->polygonChunk); }
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

void VRMaterial::setClipPlane(bool active, Vec4d equation, VRTransformPtr beacon) {
    for (int i=0; i<getNPasses(); i++) {
        auto md = mats[i];
        if (md->clipChunk == 0) md->clipChunk = ClipPlaneChunk::create();

        if (active) md->mat->addChunk(md->clipChunk);
        else md->mat->subChunk(md->clipChunk);

        md->clipChunk->setEquation(Vec4f(equation));
        md->clipChunk->setEnable  (active);
        if (beacon) md->clipChunk->setBeacon(beacon->getNode()->node);
    }
}

void VRMaterial::setWireFrame(bool b) {
    if (b) setFrontBackModes(GL_LINE, GL_LINE);
    else setFrontBackModes(GL_FILL, GL_FILL);
}

bool VRMaterial::isWireFrame() {
    auto md = mats[activePass];
    if (md->polygonChunk == 0) return false;
    return bool(md->polygonChunk->getFrontMode() == GL_LINE && md->polygonChunk->getBackMode() == GL_LINE);
}

void VRMaterial::setVideo(string vid_path) {
    auto md = mats[activePass];
    if (md->video == 0) md->video = new VRVideo( ptr() );
    md->video->open(vid_path);
}

VRVideo* VRMaterial::getVideo() { return mats[activePass]->video; }

void VRMaterial::toggleMaterial(string mat1, string mat2, bool b){
    if (b) setTexture(mat1);
    else setTexture(mat2);
}

bool VRMaterial::isCMat(MaterialMTUncountedPtr matPtr) { return (dynamic_pointer_cast<ChunkMaterial>(matPtr)); }
bool VRMaterial::isSMat(MaterialMTUncountedPtr matPtr) { return (dynamic_pointer_cast<SimpleMaterial>(matPtr)); }
bool VRMaterial::isSTMat(MaterialMTUncountedPtr matPtr) { return (dynamic_pointer_cast<SimpleTexturedMaterial>(matPtr)); }

// to access the protected member of materials
class MAC : private SimpleTexturedMaterial {
    public:

        static MaterialChunkMTRecPtr getMatChunk(Material* matPtr) {
            if (matPtr == 0) return 0;

            MaterialChunkMTRecPtr mchunk = 0;
            MaterialMTRecPtr mat = MaterialMTRecPtr(matPtr);

            SimpleMaterialMTRecPtr smat = dynamic_pointer_cast<SimpleMaterial>(mat);
            SimpleTexturedMaterialMTRecPtr stmat = dynamic_pointer_cast<SimpleTexturedMaterial>(mat);
            ChunkMaterialMTRecPtr cmat = dynamic_pointer_cast<ChunkMaterial>(mat);

            if (smat || stmat)  {
                MAC* macc = (MAC*)matPtr;
                MaterialChunkMTRecPtr mchunk = macc->_materialChunk;
                if (mchunk) return mchunk;
            }

            if (cmat) {
                for (uint i=0; i<cmat->getMFChunks()->size(); i++) {
                    StateChunkMTRecPtr chunk = cmat->getChunk(i);
                    mchunk = dynamic_pointer_cast<MaterialChunk>(chunk);
                    if (mchunk) return mchunk;
                }
            }

            return mchunk;
        }

        static BlendChunkMTRecPtr getBlendChunk(Material* matPtr) {
            MaterialMTRecPtr mat = MaterialMTRecPtr(matPtr);
            ChunkMaterialMTRecPtr cmat = dynamic_pointer_cast<ChunkMaterial>(mat);
            BlendChunkMTRecPtr bchunk = 0;
            if (cmat) {
                for (uint i=0; i<cmat->getMFChunks()->size(); i++) {
                    StateChunkMTRecPtr chunk = cmat->getChunk(i);
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
    recUndo(&VRMaterial::setTransparency, ptr(), getTransparency(), c);
    auto md = mats[activePass];
    md->colChunk->setDiffuse( toColor4f(getDiffuse(), c) );

    if (c == 1) clearTransparency();
    else enableTransparency();
    //enableTransparency();
}

void VRMaterial::enableTransparency(bool user_override) {
    force_transparency = user_override;
    auto md = mats[activePass];
    if (md->blendChunk == 0) {
        md->blendChunk = BlendChunk::create();
        md->mat->addChunk(md->blendChunk);
    }
    md->blendChunk->setSrcFactor  ( GL_SRC_ALPHA           );
    md->blendChunk->setDestFactor ( GL_ONE_MINUS_SRC_ALPHA );
}

void VRMaterial::clearTransparency(bool user_override) {
    if (user_override) force_transparency = false;
    if (force_transparency && !user_override) return;
    auto md = mats[activePass];
    md->clearChunk(md->blendChunk);
    md->clearChunk(md->depthChunk);
}

void VRMaterial::setDepthTest(int d) {
    auto md = mats[activePass];
    if (md->depthChunk == 0) {
        md->depthChunk = DepthChunk::create();
        md->depthChunk->setFunc(d); // GL_ALWAYS
        md->mat->addChunk(md->depthChunk);
    }
}

void VRMaterial::setDiffuse(string c) { setDiffuse(toColor(c)); }
void VRMaterial::setSpecular(string c) { setSpecular(toColor(c)); }
void VRMaterial::setAmbient(string c) { setAmbient(toColor(c)); }
void VRMaterial::setDiffuse(Color3f c) { mats[activePass]->colChunk->setDiffuse( toColor4f(c, getTransparency()) );}
void VRMaterial::setSpecular(Color3f c) { mats[activePass]->colChunk->setSpecular(toColor4f(c)); }
void VRMaterial::setAmbient(Color3f c) { mats[activePass]->colChunk->setAmbient(toColor4f(c)); }
void VRMaterial::setEmission(Color3f c) { mats[activePass]->colChunk->setEmission(toColor4f(c)); }
void VRMaterial::setShininess(float c) { mats[activePass]->colChunk->setShininess(c); }
void VRMaterial::setLit(bool b) { mats[activePass]->colChunk->setLit(b); updateDeferredShader(); }

Color3f VRMaterial::getDiffuse() { return toColor3f( mats[activePass]->colChunk->getDiffuse() ); }
Color3f VRMaterial::getSpecular() { return toColor3f( mats[activePass]->colChunk->getSpecular() ); }
Color3f VRMaterial::getAmbient() { return toColor3f( mats[activePass]->colChunk->getAmbient() ); }
Color3f VRMaterial::getEmission() { return toColor3f( mats[activePass]->colChunk->getEmission() ); }
float VRMaterial::getShininess() { return mats[activePass]->colChunk->getShininess(); }
float VRMaterial::getTransparency() { return mats[activePass]->colChunk->getDiffuse()[3]; }
bool VRMaterial::isLit() { return mats[activePass]->colChunk->getLit(); }

VRTexturePtr VRMaterial::getTexture(int unit) {
    auto md = mats[activePass];
    if (md->texChunks.count(unit) == 0) return 0;
    return VRTexture::create( md->texChunks[unit]->getImage() );
}

TextureObjChunkMTRecPtr VRMaterial::getTextureObjChunk(int unit) {
    if (mats[activePass]->texChunks.count(unit) == 0) return 0;
    return mats[activePass]->texChunks[unit];
}

void VRMaterial::initShaderChunk() {
    auto md = mats[activePass];
    if (md->shaderChunk != 0) return;
    md->shaderChunk = ShaderProgramChunk::create();
    md->mat->addChunk(md->shaderChunk);

    md->vProgram = ShaderProgram::createVertexShader  ();
    md->fProgram = ShaderProgram::createFragmentShader();
    md->fdProgram = ShaderProgram::createFragmentShader();
    md->gProgram = ShaderProgram::createGeometryShader();

    md->tcProgram = ShaderProgram::create();
    md->tcProgram->setShaderType(GL_TESS_CONTROL_SHADER);
    md->teProgram = ShaderProgram::create();
    md->teProgram->setShaderType(GL_TESS_EVALUATION_SHADER);

    md->shaderChunk->addShader(md->vProgram);

    if (md->deferred) md->shaderChunk->addShader(md->fdProgram);
    else              md->shaderChunk->addShader(md->fProgram);

    md->shaderChunk->addShader(md->gProgram);
    md->shaderChunk->addShader(md->tcProgram);
    md->shaderChunk->addShader(md->teProgram);

    md->vProgram->createDefaulAttribMapping();
    md->vProgram->addOSGVariable("OSGViewportSize");
}

void VRMaterial::enableShaderParameter(string name) {
    auto md = mats[activePass];
    md->vProgram->addOSGVariable(name.c_str());
}

void VRMaterial::remShaderChunk() {
    auto md = mats[activePass];
    if (md->shaderChunk == 0) return;
    md->mat->subChunk(md->shaderChunk);
    md->vProgram = 0;
    md->fProgram = 0;
    md->fdProgram = 0;
    md->gProgram = 0;
    md->tcProgram = 0;
    md->teProgram = 0;
    md->shaderChunk = 0;
}

ShaderProgramMTRecPtr VRMaterial::getShaderProgram() { return mats[activePass]->vProgram; }

void VRMaterial::setDefaultVertexShader() {
    auto md = mats[activePass];
    int texD = md->getTextureDimension();

    string vp;
    vp += "#version 120\n";
    vp += "attribute vec4 osg_Vertex;\n";
    //vp += "attribute vec3 osg_Normal;\n";
    //vp += "attribute vec4 osg_Color;\n";
    if (texD == 2) vp += "attribute vec2 osg_MultiTexCoord0;\n";
    if (texD == 3) vp += "attribute vec3 osg_MultiTexCoord0;\n";
    vp += "void main(void) {\n";
    //vp += "  gl_Normal = gl_NormalMatrix * osg_Normal;\n";
    if (texD == 2) vp += "  gl_TexCoord[0] = vec4(osg_MultiTexCoord0,0.0,0.0);\n";
    if (texD == 3) vp += "  gl_TexCoord[0] = vec4(osg_MultiTexCoord0,0.0);\n";
    vp += "  gl_Position    = gl_ModelViewProjectionMatrix*osg_Vertex;\n";
    vp += "}\n";

    setVertexShader(vp, "defaultVS");
}

// type: GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, ...
void VRMaterial::checkShader(int type, string shader, string name) {
    auto gm = VRGuiManager::get(false);
    if (!gm) return;
    auto errC = gm->getConsole("Errors");
    if (!errC) return;

    if (!glXGetCurrentContext()) return;

    GLuint shaderObject = glCreateShader(type);
    int N = shader.size();
    const char* str = shader.c_str();
    glShaderSourceARB(shaderObject, 1, &str, &N);
    glCompileShaderARB(shaderObject);

    GLint compiled;
    glGetObjectParameterivARB(shaderObject, GL_COMPILE_STATUS, &compiled);
    if (!compiled) errC->write( "Shader "+name+" of material "+getName()+" did not compiled!\n");

    GLint blen = 0;
    GLsizei slen = 0;
    glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH , &blen);
    if (blen > 1) {
        GLchar* compiler_log = (GLchar*)malloc(blen);
        glGetInfoLogARB(shaderObject, blen, &slen, compiler_log);
        errC->write( "Shader "+name+" of material "+getName()+" warnings and errors:\n");
        errC->write( string(compiler_log));
        free(compiler_log);
    }
}

void VRMaterial::forceShaderUpdate() {
    string s = mats[activePass]->vProgram->getProgram();
    mats[activePass]->vProgram->setProgram(s.c_str());
}

void VRMaterial::setVertexShader(string s, string name) {
    initShaderChunk();
    mats[activePass]->vProgram->setProgram(s.c_str());
    checkShader(GL_VERTEX_SHADER, s, name);
    mats[activePass]->tmpDeferredShdr = false;
}

void VRMaterial::setFragmentShader(string s, string name, bool deferred) {
    initShaderChunk();
    if (deferred) mats[activePass]->fdProgram->setProgram(s.c_str());
    else          mats[activePass]->fProgram->setProgram(s.c_str());
    checkShader(GL_FRAGMENT_SHADER, s, name);
    mats[activePass]->tmpDeferredShdr = false;
}

void VRMaterial::setGeometryShader(string s, string name) {
    initShaderChunk();
    mats[activePass]->gProgram->setProgram(s.c_str());
    checkShader(GL_GEOMETRY_SHADER, s, name);
}

void VRMaterial::setTessControlShader(string s, string name) {
    initShaderChunk();
    mats[activePass]->tcProgram->setProgram(s.c_str());
    forceShaderUpdate();
    checkShader(GL_TESS_CONTROL_SHADER, s, name);
}

void VRMaterial::setTessEvaluationShader(string s, string name) {
    initShaderChunk();
    mats[activePass]->teProgram->setProgram(s.c_str());
    forceShaderUpdate();
    checkShader(GL_TESS_EVALUATION_SHADER, s, name);
}

string readFile(string path) {
    ifstream file(path.c_str());
    string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return str;
}

void VRMaterial::readVertexShader(string s) { setVertexShader(readFile(s), s); }
void VRMaterial::readFragmentShader(string s, bool deferred) { setFragmentShader(readFile(s), s, deferred); }
void VRMaterial::readGeometryShader(string s) { setGeometryShader(readFile(s), s); }
void VRMaterial::readTessControlShader(string s) { setTessControlShader(readFile(s), s); }
void VRMaterial::readTessEvaluationShader(string s) { setTessEvaluationShader(readFile(s), s); }

string VRMaterial::getVertexShader() { return ""; } // TODO
string VRMaterial::getFragmentShader() { return ""; }
string VRMaterial::getGeometryShader() { return ""; }

void VRMaterial::setVertexScript(string script) {
    mats[activePass]->vertexScript = script;
    VRScriptPtr scr = VRScene::getCurrent()->getScript(script);
    if (scr) setVertexShader(scr->getCore(), script);
}

void VRMaterial::setFragmentScript(string script, bool deferred) {
    if (deferred) mats[activePass]->fragmentDScript = script;
    else mats[activePass]->fragmentScript = script;
    VRScriptPtr scr = VRScene::getCurrent()->getScript(script);
    if (scr) setFragmentShader(scr->getCore(), script, deferred);
    else cout << " Warning: script "+script+" not found!\n";
}

void VRMaterial::setGeometryScript(string script) {
    mats[activePass]->geometryScript = script;
    VRScriptPtr scr = VRScene::getCurrent()->getScript(script);
    if (scr) setGeometryShader(scr->getCore(), script);
}

void VRMaterial::setTessControlScript(string script) {
    mats[activePass]->tessControlScript = script;
    VRScriptPtr scr = VRScene::getCurrent()->getScript(script);
    if (scr) setTessControlShader(scr->getCore(), script);
}

void VRMaterial::setTessEvaluationScript(string script) {
    mats[activePass]->tessEvalScript = script;
    VRScriptPtr scr = VRScene::getCurrent()->getScript(script);
    if (scr) setTessEvaluationShader(scr->getCore(), script);
}

string VRMaterial::getFragmentScript(bool deferred) {
    return deferred ? mats[activePass]->fragmentDScript : mats[activePass]->fragmentScript;
}
string VRMaterial::getVertexScript() { return mats[activePass]->vertexScript; }
string VRMaterial::getGeometryScript() { return mats[activePass]->geometryScript; }
string VRMaterial::getTessControlScript() { return mats[activePass]->tessControlScript; }
string VRMaterial::getTessEvaluationScript() { return mats[activePass]->tessEvalScript; }

Color3f VRMaterial::toColor(string c) {
    static map<string, Color3f> colorMap;

    if (colorMap.size() == 0) {
        colorMap["red"] = Color3f(1,0,0);
        colorMap["green"] = Color3f(0,1,0);
        colorMap["blue"] = Color3f(0,0,1);
        colorMap["yellow"] = Color3f(1,1,0);
        colorMap["brown"] = Color3f(0.5,0.25,0);
        colorMap["orange"] = Color3f(1,0.6,0);
        colorMap["magenta"] = Color3f(1,0,1);
        colorMap["cyan"] = Color3f(0,1,1);
        colorMap["black"] = Color3f(0,0,0);
        colorMap["white"] = Color3f(1,1,1);
    }

    return colorMap.count(c) ? colorMap[c] : Color3f();
}

OSG_END_NAMESPACE;
