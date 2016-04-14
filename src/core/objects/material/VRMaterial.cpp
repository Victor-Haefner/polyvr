#include "VRMaterial.h"

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

#include "core/objects/VRTransform.h"
#include "core/objects/material/VRTexture.h"
#include "core/utils/toString.h"
#include "core/utils/VRUndoInterfaceT.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/scripting/VRScript.h"
#include "VRVideo.h"
#include "core/tools/VRQRCode.h"
#include <libxml++/nodes/element.h>
#include <boost/filesystem.hpp>

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
    ShaderProgramMTRecPtr gProgram;
    VRVideo* video = 0;
    bool deffered = false;

    string vertexScript;
    string fragmentScript;
    string geometryScript;

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
        deffered = false;

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
        if (gProgram) { m->gProgram = dynamic_pointer_cast<ShaderProgram>(gProgram->shallowCopy()); m->shaderChunk->addShader(m->gProgram); }
        if (video) ; // TODO

        m->vertexScript = vertexScript;
        m->fragmentScript = fragmentScript;
        m->geometryScript = geometryScript;

        return m;
    }

    template<typename T>
    void clearChunk(T& c) {
        if (c) { mat->subChunk(c); c = 0; }
    }
};

map<string, VRMaterialWeakPtr> VRMaterial::materials;
map<MaterialMTRecPtr, VRMaterialWeakPtr> VRMaterial::materialsByPtr;

VRMaterial::VRMaterial(string name) : VRObject(name) {
    auto scene = VRSceneManager::getCurrent();
    if (scene) deferred = scene->getDefferedShading();
    addAttachment("material", 0);
    passes = MultiPassMaterial::create();
    addPass();
    activePass = 0;

    MaterialGroupMTRecPtr group = MaterialGroup::create();
    group->setMaterial(passes);
    setCore(group, "Material");

    //store("diffuse", &diffuse);
    //store("specular", &specular);
    //store("ambient", &ambient);
}

VRMaterial::~VRMaterial() { for (auto m : mats) delete m; }

VRMaterialPtr VRMaterial::ptr() { return static_pointer_cast<VRMaterial>( shared_from_this() ); }
VRMaterialPtr VRMaterial::create(string name) {
    auto p = shared_ptr<VRMaterial>(new VRMaterial(name) );
    materials[p->getName()] = p;
    return p;
}

string VRMaterial::constructShaderVP(VRMatData* data) {
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

string VRMaterial::constructShaderFP(VRMatData* data) {
    int texD = data->getTextureDimension();

    string fp;
    fp += "#version 120\n";
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
    fp += "  gl_FragData[1] = vec4(normalize(vertNorm), 0);\n";
    fp += "  gl_FragData[2] = vec4(diffCol, 0);\n";
    fp += "}\n";
    return fp;
}

void VRMaterial::setDeffered(bool b) {
    deferred = b;
    if (b) {
        for (uint i=0; i<mats.size(); i++) {
            if (mats[i]->shaderChunk != 0) continue;
            mats[i]->deffered = true;
            setActivePass(i);
            setVertexShader( constructShaderVP(mats[i]) );
            setFragmentShader( constructShaderFP(mats[i]) );
        }
    } else {
        for (uint i=0; i<mats.size(); i++) {
            if (!mats[i]->deffered) continue;
            mats[i]->deffered = false;
            setActivePass(i);
            remShaderChunk();
        }
    }
}

void VRMaterial::clearAll() {
    materials.clear();
    materialsByPtr.clear();
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
int VRMaterial::getNPasses() { return passes->getNPasses(); }

int VRMaterial::addPass() {
    activePass = getNPasses();
    VRMatData* md = new VRMatData();
    md->reset();
    passes->addMaterial(md->mat);
    mats.push_back(md);
    setDeffered(deferred);
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
void VRMaterial::appendPasses(VRMaterialPtr mat) {
    for (int i=0; i<mat->getNPasses(); i++) {
        VRMatData* md = mat->mats[i]->copy();
        passes->addMaterial(md->mat);
        mats.push_back(md);
    }
}

void VRMaterial::prependPasses(VRMaterialPtr mat) {
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

VRObjectPtr VRMaterial::copy(vector<VRObjectPtr> children) {
    VRMaterialPtr mat = VRMaterial::create(getBaseName());
    cout << "Warning: VRMaterial::copy not implemented!\n";
    // TODO: copy all the stuff
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

MultiPassMaterialMTRecPtr VRMaterial::getMaterial() { return passes; }
ChunkMaterialMTRecPtr VRMaterial::getMaterial(int i) { return mats[i]->mat; }

void VRMaterial::setTextureParams(int min, int mag, int envMode, int wrapS, int wrapT, int unit) {
    auto md = mats[activePass];
    if (md->texChunks.count(unit) == 0) { md->texChunks[unit] = TextureObjChunk::create(); md->mat->addChunk(md->texChunks[unit], unit); }
    if (md->envChunks.count(unit) == 0) { md->envChunks[unit] = TextureEnvChunk::create(); md->mat->addChunk(md->envChunks[unit], unit); }

    md->texChunks[unit]->setMinFilter (min);
    md->texChunks[unit]->setMagFilter (mag);
    md->envChunks[unit]->setEnvMode (envMode);
    md->texChunks[unit]->setWrapS (wrapS);
    md->texChunks[unit]->setWrapT (wrapT);
}

void VRMaterial::setTexture(string img_path, bool alpha, int unit) { // TODO: improve with texture map
    if (boost::filesystem::exists(img_path))
        img_path = boost::filesystem::canonical(img_path).string();
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
    if (alpha && img->getImage()->hasAlphaChannel() && md->blendChunk == 0) {
        md->blendChunk = BlendChunk::create();
        md->mat->addChunk(md->blendChunk);
    }

    if (alpha && img->getImage()->hasAlphaChannel()) {
        md->blendChunk->setSrcFactor  ( GL_SRC_ALPHA           );
        md->blendChunk->setDestFactor ( GL_ONE_MINUS_SRC_ALPHA );
    }

    if (img->getInternalFormat() != -1) md->texChunks[unit]->setInternalFormat(img->getInternalFormat());
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

void VRMaterial::setQRCode(string s, Vec3f fg, Vec3f bg, int offset) {
    createQRCode(s, ptr(), fg, bg, offset);
    setTextureParams(GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST);
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

void VRMaterial::setClipPlane(bool active, Vec4f equation, VRTransformPtr beacon) {
    for (int i=0; i<getNPasses(); i++) {
        auto md = mats[i];
        if (md->clipChunk == 0) md->clipChunk = ClipPlaneChunk::create();

        if (active) md->mat->addChunk(md->clipChunk);
        else md->mat->subChunk(md->clipChunk);

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
    recUndo(&VRMaterial::setTransparency, this, getTransparency(), c);
    auto md = mats[activePass];
    md->colChunk->setDiffuse( toColor4f(getDiffuse(), c) );

    if (c == 1) clearTransparency();
    else enableTransparency();
    //enableTransparency();
}

void VRMaterial::enableTransparency() {
    auto md = mats[activePass];
    if (md->blendChunk == 0) {
        md->blendChunk = BlendChunk::create();
        md->blendChunk->setSrcFactor  ( GL_SRC_ALPHA           );
        md->blendChunk->setDestFactor ( GL_ONE_MINUS_SRC_ALPHA );
        md->mat->addChunk(md->blendChunk);
    }
}

void VRMaterial::clearTransparency() {
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
    md->gProgram = ShaderProgram::createGeometryShader();
    md->shaderChunk->addShader(md->vProgram);
    md->shaderChunk->addShader(md->fProgram);
    md->shaderChunk->addShader(md->gProgram);

    md->vProgram->createDefaulAttribMapping();
    md->vProgram->addOSGVariable("OSGViewportSize");
}

void VRMaterial::remShaderChunk() {
    auto md = mats[activePass];
    if (md->shaderChunk == 0) return;
    md->mat->subChunk(md->shaderChunk);
    md->vProgram = 0;
    md->fProgram = 0;
    md->gProgram = 0;
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

    setVertexShader(vp);
}

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

    setTextureParams(Min, Mag);
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
