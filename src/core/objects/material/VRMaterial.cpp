#include "VRMaterial.h"
#include "OSGMaterial.h"
#ifndef WITHOUT_GTK
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"
#endif
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
#ifndef WITHOUT_AV
#include "VRVideo.h"
#endif
#include "core/tools/VRQRCode.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/VRLogger.h"

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
#include <OpenSG/OSGTextureTransformChunk.h>
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
#include <OpenSG/OSGCubeTextureObjChunk.h>
#include <OpenSG/OSGGLEXT.h>
#include <OpenSG/OSGShaderProgramVariables.h>
#include <OpenSG/OSGShaderValueVariable.h>
#include <OpenSG/OSGShaderProcVariable.h>

#ifdef OSG_OGL_ES2
#elif defined(_WIN32)
#else
#include <GL/glx.h>
#endif

using namespace OSG;

template<> string typeName(const VRMaterial& o) { return "Material"; }

namespace OSG {
struct VRMatData {
    ChunkMaterialMTRecPtr mat;
    MaterialChunkMTRecPtr colChunk;
    BlendChunkMTRecPtr blendChunk;
    DepthChunkMTRecPtr depthChunk;
    map<int, TextureObjChunkMTRecPtr>       texChunks;
    map<int, TextureEnvChunkMTRecPtr>       envChunks;
    map<int, CubeTextureObjChunkMTRecPtr>   ctxChunks;
    map<int, TexGenChunkMTRecPtr>           genChunks;
    map<int, TextureTransformChunkMTRecPtr> tnsChunks;
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
    VRVideoPtr video;
    bool deferred = false;
    bool tmpDeferredShdr = false;
    bool tmpOGLESShdr = false;

#ifdef OSG_OGL_ES2
    ShaderProgramChunkMTRecPtr shaderFailChunk;
    bool vertShaderFail = false;
    bool fragShaderFail = false;
#endif

    string vertexScript;
    string fragmentScript;
    string fragmentDScript;
    string geometryScript;
    string tessControlScript;
    string tessEvalScript;

    ~VRMatData() {}

    void regChunk(StateChunkMTRecPtr chunk, int unit, int attachmentID) {
        mat->addChunk(chunk, unit);
        mat->addAttachment(chunk, attachmentID);
    }

    void addTextureChunks(int unit) {
        if (texChunks.count(unit) == 0) {
            texChunks[unit] = TextureObjChunk::create();
            regChunk(texChunks[unit], unit, 4*10+unit);
        }
        if (envChunks.count(unit) == 0) {
            envChunks[unit] = TextureEnvChunk::create();
            regChunk(envChunks[unit], unit, 5*10+unit);
            envChunks[unit]->setEnvMode(GL_MODULATE);
        }
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
        regChunk(colChunk, -2, 0);
        if (colChunk->getId() == 3060) cout << " -------------------- MaterialChunk " << colChunk->getId() << endl;
        twoSidedChunk = TwoSidedLightingChunk::create();
        regChunk(twoSidedChunk, -2, 1);
        texChunks.clear();
        envChunks.clear();
        genChunks.clear();
        ctxChunks.clear();
        tnsChunks.clear();
        blendChunk = 0;
        depthChunk = 0;
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
        tmpOGLESShdr = true;

        colChunk->setDiffuse( Color4f(1, 1, 1, 1) );
        colChunk->setAmbient( Color4f(0.3, 0.3, 0.3, 1) );
        colChunk->setSpecular( Color4f(1, 1, 1, 1) );
        colChunk->setShininess( 50 );
    }

    VRMatDataPtr copy() {
        VRMatDataPtr m = VRMatDataPtr( new VRMatData() );
        m->mat = ChunkMaterial::create();

        if (colChunk) { m->colChunk = dynamic_pointer_cast<MaterialChunk>(colChunk->shallowCopy()); m->regChunk(m->colChunk, -2, 0); }
        if (blendChunk) { m->blendChunk = dynamic_pointer_cast<BlendChunk>(blendChunk->shallowCopy()); m->regChunk(m->blendChunk, -2, 6); }
        if (depthChunk) { m->depthChunk = dynamic_pointer_cast<DepthChunk>(depthChunk->shallowCopy()); m->regChunk(m->depthChunk, -2, 8); }
        for (auto t : texChunks) { TextureObjChunkMTRecPtr mt = dynamic_pointer_cast<TextureObjChunk>(t.second->shallowCopy()); m->texChunks[t.first] = mt; m->regChunk(mt, t.first, 40+t.first); }
        for (auto t : envChunks) { TextureEnvChunkMTRecPtr mt = dynamic_pointer_cast<TextureEnvChunk>(t.second->shallowCopy()); m->envChunks[t.first] = mt; m->regChunk(mt, t.first, 50+t.first); }
        for (auto t : genChunks) { TexGenChunkMTRecPtr     mt = dynamic_pointer_cast<TexGenChunk>    (t.second->shallowCopy()); m->genChunks[t.first] = mt; m->regChunk(mt, t.first, 60+t.first); }
        for (auto t : ctxChunks) { CubeTextureObjChunkMTRecPtr     mt = dynamic_pointer_cast<CubeTextureObjChunk>    (t.second->shallowCopy()); m->ctxChunks[t.first] = mt; m->regChunk(mt, t.first, 70+t.first); }
        for (auto t : tnsChunks) { TextureTransformChunkMTRecPtr   mt = dynamic_pointer_cast<TextureTransformChunk>  (t.second->shallowCopy()); m->tnsChunks[t.first] = mt; m->regChunk(mt, t.first, 80+t.first); }
        if (lineChunk) { m->lineChunk = dynamic_pointer_cast<LineChunk>(lineChunk->shallowCopy()); m->regChunk(m->lineChunk, -2, 2); }
        if (pointChunk) { m->pointChunk = dynamic_pointer_cast<PointChunk>(pointChunk->shallowCopy()); m->regChunk(m->pointChunk, -2, 3); }
        if (polygonChunk) { m->polygonChunk = dynamic_pointer_cast<PolygonChunk>(polygonChunk->shallowCopy()); m->regChunk(m->polygonChunk, -2, 11); }
        if (twoSidedChunk) { m->twoSidedChunk = dynamic_pointer_cast<TwoSidedLightingChunk>(twoSidedChunk->shallowCopy()); m->regChunk(m->twoSidedChunk, -2, 1); }
        if (clipChunk) { m->clipChunk = dynamic_pointer_cast<ClipPlaneChunk>(clipChunk->shallowCopy()); m->regChunk(m->clipChunk, -2, 10); }
        if (stencilChunk) { m->stencilChunk = dynamic_pointer_cast<StencilChunk>(stencilChunk->shallowCopy()); m->regChunk(m->stencilChunk, -2, 18); }
        if (shaderChunk) { m->shaderChunk = ShaderProgramChunk::create(); m->regChunk(m->shaderChunk, -2, 7); }

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
        //if (video) ; // TODO

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
        if (c) {
            mat->subChunk(c);
            c = 0;
            mat->subAttachment(c);
        }
    }
};
}

map<string, VRMaterialWeakPtr> VRMaterial::materials;
map<MaterialMTRecPtr, VRMaterialWeakPtr> VRMaterial::materialsByPtr;
map<size_t, size_t> VRMaterial::fieldContainerMap;

VRMaterial::VRMaterial(string name) : VRObject(name) {}

VRMaterial::~VRMaterial() {}

VRMaterialPtr VRMaterial::ptr() { return static_pointer_cast<VRMaterial>( shared_from_this() ); }

VRMaterialPtr VRMaterial::create(string name) {
    auto p = VRMaterialPtr(new VRMaterial(name) );
    p->init();
    materials[p->getName()] = p;
#ifdef OSG_OGL_ES2
    p->updateOGL2Shader(); // TODO: find a better place!
#endif
    return p;
}

void VRMaterial::init() {
    auto scene = VRScene::getCurrent();
    if (scene) deferred = scene->getDefferedShading();
    addAttachment("material", 0);
    passes = OSGMaterial::create( MultiPassMaterial::create() );
    ::setName(passes->mat, getName());
    addPass();
    activePass = 0;

    MaterialGroupMTRecPtr group = MaterialGroup::create();
    group->setMaterial(passes->mat);
    setCore(OSGCore::create(group), "Material");

    //store("diffuse", &diffuse);
    //store("specular", &specular);
    //store("ambient", &ambient);
}

void VRMaterial::setDefaultVertexShader() {
    string vp = constructShaderVP();
    setVertexShader(vp, "defaultVS");
}

string vertFailShader =
"attribute vec4 osg_Vertex;\n"
"uniform mat4 OSGModelViewProjectionMatrix;\n"
"void main(void) {\n"
"  gl_Position = OSGModelViewProjectionMatrix * osg_Vertex;\n"
"}\n";

string fragFailShader =
"precision mediump float;\n"
"void main(void) {\n"
"  gl_FragColor = vec4(0.0,0.8,1.0,1.0);\n"
"}\n";

string VRMaterial::constructShaderVP(VRMatDataPtr data) {
    if (!data) data = mats[activePass];
    int texD = data->getTextureDimension();
    //bool hasVertCols = ;

    string vp;
#ifdef OSG_OGL_ES2
    vp += "attribute vec4 osg_Vertex;\n";
    vp += "attribute vec3 osg_Normal;\n";
    vp += "attribute vec4 osg_Color;\n";
    if (texD == 2) vp += "attribute vec2 osg_MultiTexCoord0;\n";
    vp += "uniform mat4 OSGModelViewProjectionMatrix;\n";
    vp += "uniform mat4 OSGNormalMatrix;\n";
    vp += "varying vec4 vertPos;\n";
    vp += "varying vec3 vertNorm;\n";
    vp += "varying vec4 color;\n";
	if (texD == 2) vp += "varying vec2 texCoord;\n";
    vp += "void main(void) {\n";
    vp += "  vertNorm = (OSGNormalMatrix * vec4(osg_Normal,1.0)).xyz;\n";
    if (texD == 2) vp += "  texCoord = osg_MultiTexCoord0;\n";
    vp += "  color = osg_Color;\n";
    vp += "  gl_Position = OSGModelViewProjectionMatrix * osg_Vertex;\n";
    vp += "}\n";
#else
    vp += "#version 120\n";
    vp += "attribute vec4 osg_Vertex;\n";
    vp += "attribute vec3 osg_Normal;\n";
    //vp += "attribute vec4 osg_Color;\n";
    if (texD == 2) vp += "attribute vec2 osg_MultiTexCoord0;\n";
    if (texD == 3) vp += "attribute vec3 osg_MultiTexCoord0;\n";
    vp += "varying vec4 vertPos;\n";
    vp += "varying vec3 vertNorm;\n";
    vp += "varying vec4 color;\n";
    vp += "void main(void) {\n";
    vp += "  vertPos = gl_ModelViewMatrix * osg_Vertex;\n";
    vp += "  vertNorm = gl_NormalMatrix * osg_Normal;\n";
    if (texD == 2) vp += "  gl_TexCoord[0] = vec4(osg_MultiTexCoord0,0.0,0.0);\n";
    if (texD == 3) vp += "  gl_TexCoord[0] = vec4(osg_MultiTexCoord0,0.0);\n";
    vp += "  color  = gl_Color;\n";
    vp += "  gl_Position    = gl_ModelViewProjectionMatrix*osg_Vertex;\n";
    vp += "}\n";
#endif

    return vp;
}

string VRMaterial::constructShaderFP(VRMatDataPtr data, bool deferred, int forcedTextureDim) {
    if (!data) data = mats[activePass];
    int texD = forcedTextureDim;
    if (texD == -1) texD = data->getTextureDimension();

    string fp;
#ifdef OSG_OGL_ES2
    fp += "precision mediump float;\n";
    fp += "varying vec3 vertNorm;\n";
    fp += "varying vec4 color;\n";
	if (texD == 2) fp += "varying vec2 texCoord;\n";
    fp += "uniform int isLit;\n";
    fp += "uniform vec4 mat_diffuse;\n";
    fp += "uniform vec4 mat_ambient;\n";
    fp += "uniform vec4 mat_specular;\n";
    if (texD == 2) fp += "uniform sampler2D tex0;\n";
    fp += "void main(void) {\n";
    fp += "  vec3  n = normalize(vertNorm);\n";
    fp += "  vec3  light = normalize( vec3(0.8,1.0,0.5) );\n";
    fp += "  float NdotL = max(dot( n, light ), 0.0);\n";
    if (texD == 2) fp += "  vec4 diffCol = texture2D(tex0, texCoord);\n";
//    if (texD == 2) fp += "  vec4 diffCol = vec4(texCoord.x, texCoord.y, 0.0, 1.0);\n";
    else fp += "  vec4 diffCol = color;\n";
    fp += "  vec4  ambient = mat_ambient * diffCol;\n";
    fp += "  vec4  diffuse = NdotL * diffCol;\n";
    fp += "  vec4  specular = mat_specular * 0.0;\n";
    fp += "  if (isLit == 0) gl_FragColor = mat_diffuse * diffCol;\n";
    fp += "  else gl_FragColor = ambient + diffuse + specular;\n";
    fp += "}\n";
#else
    fp += "#version 120\n";
    if (deferred) fp += "uniform int isLit;\n";
    fp += "varying vec4 vertPos;\n";
    fp += "varying vec3 vertNorm;\n";
    fp += "varying vec4 color;\n";
    if (texD == 2) fp += "uniform sampler2D tex0;\n";
    if (texD == 3) fp += "uniform sampler3D tex0;\n";

    if (!deferred) {
        fp += "void applyLightning() {\n";
        fp += " vec3  n = normalize(vertNorm);\n";
        fp += " vec3  light = normalize( gl_LightSource[0].position.xyz );\n";// directional light
        fp += " float NdotL = max(dot( n, light ), 0.0);\n";
        fp += " vec4  ambient = gl_LightSource[0].ambient * color;\n";
        fp += " vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;\n";
        fp += " float NdotHV = max(dot(n, normalize(gl_LightSource[0].halfVector.xyz)),0.0);\n";
        fp += " vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );\n";
        fp += " gl_FragColor = ambient + diffuse + specular;\n";
        fp += "}\n";
    }

    fp += "void main(void) {\n";
    fp += "  vec3 pos = vertPos.xyz / vertPos.w;\n";
    if (texD == 0) fp += "  vec4 diffCol = color;\n";
    if (texD == 2) fp += "  vec4 diffCol = texture2D(tex0, gl_TexCoord[0].xy);\n";
    if (texD == 3) fp += "  vec4 diffCol = texture3D(tex0, gl_TexCoord[0].xyz);\n";
    if (deferred) {
        fp += "  if (diffCol.a < 0.1) discard;\n";
        fp += "  diffCol.rgb = mix(vec3(0.5), diffCol.rgb, diffCol.a);\n";
        fp += "  gl_FragData[0] = vec4(pos, 1.0);\n";
        fp += "  gl_FragData[1] = vec4(normalize(vertNorm), isLit);\n";
        fp += "  gl_FragData[2] = vec4(diffCol.rgb, 0);\n";
    } else {
        fp += "  applyLightning();\n";
    }
    fp += "}\n";
#endif

    return fp;
}

void VRMaterial::updateOGL2Parameters() {
#ifdef OSG_OGL_ES2
    auto a = getAmbient();
    auto d = getDiffuse();
    auto s = getSpecular();
    setShaderParameter("isLit", int(isLit()));
    setShaderParameter("mat_ambient", Vec4f(a[0], a[1], a[2], 1.0));
    setShaderParameter("mat_diffuse", Vec4f(d[0], d[1], d[2], 1.0));
    setShaderParameter("mat_specular", Vec4f(s[0], s[1], s[2], 1.0));
    //setFrontBackModes(GL_NONE, GL_FILL);
#endif
}

void VRMaterial::updateOGL2Shader() {
    auto m = mats[activePass];
    if (m->tmpOGLESShdr) {
        initShaderChunk();
        string s = constructShaderVP(m);
        m->vProgram->setProgram(s.c_str());
        checkShader(GL_VERTEX_SHADER, s, "ogl2VS");

        s = constructShaderFP(m);
        m->fProgram->setProgram(s.c_str());
        checkShader(GL_FRAGMENT_SHADER, s, "ogl2FS");
    }
    updateOGL2Parameters();
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
    for (unsigned int i=0; i<mats.size(); i++) {
        setActivePass(i);
        if (b) {
            if (mats[i]->shaderChunk == 0) {
                mats[i]->tmpDeferredShdr = true;
                updateDeferredShader();
            }
        } else if (mats[i]->tmpDeferredShdr) remShaderChunk();
        mats[i]->toggleDeferredShader(b, getName());
        setTransparency( getTransparency() );
    }
    setActivePass(a);
}

void VRMaterial::testFix() {
    /*auto m = mats[activePass];

    string s = constructShaderVP(m);
    m->vProgram->setProgram(s.c_str());
    checkShader(GL_VERTEX_SHADER, s, "defferedVS");

    s = constructShaderFP(m);
    m->fdProgram->setProgram(s.c_str());
    checkShader(GL_FRAGMENT_SHADER, s, "defferedFS");

    setShaderParameter("isLit", int(isLit()));*/
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
    passes->mat->addAttachment(md->mat);
    mats.push_back(md);
    setDeferred(deferred);
    return activePass;
}

void VRMaterial::remPass(int i) {
    if (i < 0 || i >= getNPasses()) return;
    passes->mat->subAttachment(passes->mat->getMaterials(i));
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
    if (md->stencilChunk == 0) { md->stencilChunk = StencilChunk::create(); md->regChunk(md->stencilChunk, -2, 18); }

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

VRObjectPtr VRMaterial::copy(vector<VRObjectPtr> children) {
    VRMaterialPtr mat = VRMaterial::create(getBaseName());
    mat->force_transparency = force_transparency;
    mat->deferred = deferred;
    mat->remPass(0);
    mat->appendPasses( ptr() );
    mat->activePass = activePass;
    return mat;
}

void VRMaterial::setLineWidth(int w, bool smooth) {
    auto md = mats[activePass];
    if (md->lineChunk == 0) {
        md->lineChunk = LineChunk::create();
        md->regChunk(md->lineChunk, -2, 2);
    }
    md->lineChunk->setWidth(w);
    md->lineChunk->setSmooth(smooth);
}

void VRMaterial::setPointSize(int s, bool smooth) {
    auto md = mats[activePass];
    if (md->pointChunk == 0) {
        md->pointChunk = PointChunk::create();
        md->regChunk(md->pointChunk, -2, 3);
    }
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
        for (unsigned int i=0; i<cmat->getMFChunks()->size(); i++) {
            StateChunkMTRecPtr chunk = cmat->getChunk(i);
            int unit = -2; cmat->getChunkSlot(chunk, unit);

            MaterialChunkMTRecPtr mc = dynamic_pointer_cast<MaterialChunk>(chunk);
            BlendChunkMTRecPtr bc = dynamic_pointer_cast<BlendChunk>(chunk);
            TextureEnvChunkMTRecPtr ec = dynamic_pointer_cast<TextureEnvChunk>(chunk);
            TextureObjChunkMTRecPtr tc = dynamic_pointer_cast<TextureObjChunk>(chunk);
            DepthChunkMTRecPtr dc = dynamic_pointer_cast<DepthChunk>(chunk);
            TwoSidedLightingChunkMTRecPtr tsc = dynamic_pointer_cast<TwoSidedLightingChunk>(chunk);
            ShaderProgramChunkMTRecPtr sp = dynamic_pointer_cast<ShaderProgramChunk>(chunk);
            LineChunkMTRecPtr lp = dynamic_pointer_cast<LineChunk>(chunk);
            PointChunkMTRecPtr pp = dynamic_pointer_cast<PointChunk>(chunk);

            if (mc) { md->mat->subChunk(md->colChunk); md->colChunk = mc; mc->setBackMaterial(false); md->regChunk(mc,unit,0); continue; }
            if (bc) { md->blendChunk = bc; md->regChunk(bc,unit, 6); continue; }
            if (ec) { md->envChunks[unit] = ec; md->regChunk(ec,unit, 5*10+unit); continue; }
            if (tc) { md->texChunks[unit] = tc; md->regChunk(tc,unit, 4*10+unit); continue; }
            if (dc) { md->depthChunk = dc; md->regChunk(dc,unit, 8); continue; }
            if (tsc) { md->mat->subChunk(md->twoSidedChunk); md->twoSidedChunk = tsc; md->regChunk(tsc,unit, 1); continue; }
            if (sp) { md->shaderChunk = sp; md->regChunk(sp,unit, 7); continue; }
            if (lp) { md->lineChunk = lp; md->regChunk(lp,unit, 2); continue; }
            if (pp) { md->pointChunk = pp; md->regChunk(pp,unit, 3); continue; }

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
    md->addTextureChunks(unit);

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
    else { VRLog::wrn("PyAPI", "Material '" + getName() + "' setTexture failed, path invalid: '" + img_path + "'"); return; }
    auto tex = VRTexture::create();
    tex->read(img_path);
    setTexture(tex, alpha, unit);
}

void VRMaterial::setTexture(VRTexturePtr img, bool alpha, int unit) {
    if (img == 0) return;
    if (img->getImage() == 0) return;

    auto md = mats[activePass];
    md->addTextureChunks(unit);

    //md->texture = img;
    md->texChunks[unit]->setImage(img->getImage());
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
    md->addTextureChunks(unit);
    return md->texChunks[unit];
}

void VRMaterial::setTexture(TextureObjChunkMTRefPtr texChunk, int unit) {
    auto md = mats[activePass];
    md->texChunks[unit] = texChunk;
    md->envChunks[unit] = TextureEnvChunk::create();
    md->envChunks[unit]->setEnvMode(GL_MODULATE);
    md->regChunk(md->texChunks[unit], unit, 40+unit);
    md->regChunk(md->envChunks[unit], unit, 50+unit);
}

void VRMaterial::setTextureAndUnit(VRTexturePtr img, int unit) {
    if (img == 0) return;
    auto texChunk = getTexChunk(unit);
    texChunk->setImage(img->getImage());
}

void VRMaterial::setMappingBeacon(VRObjectPtr obj, int unit) {
    auto md = mats[activePass];
    if (!md->genChunks.count(unit)) { md->genChunks[unit] = TexGenChunk::create(); md->regChunk(md->genChunks[unit], unit, 19); }
    md->genChunks[unit]->setSBeacon(obj->getNode()->node);
    md->genChunks[unit]->setTBeacon(obj->getNode()->node);
    md->genChunks[unit]->setRBeacon(obj->getNode()->node);
    md->genChunks[unit]->setQBeacon(obj->getNode()->node);
}

void VRMaterial::setMappingPlanes(Vec4d p1, Vec4d p2, Vec4d p3, Vec4d p4, int unit) {
    auto md = mats[activePass];
    if (!md->genChunks.count(unit)) { md->genChunks[unit] = TexGenChunk::create(); md->regChunk(md->genChunks[unit], unit, 18); }
    md->genChunks[unit]->setGenFuncSPlane(Vec4f(p1)); // GL_REFLECTION_MAP
    md->genChunks[unit]->setGenFuncTPlane(Vec4f(p2));
    md->genChunks[unit]->setGenFuncRPlane(Vec4f(p3));
    md->genChunks[unit]->setGenFuncQPlane(Vec4f(p4));
}

void VRMaterial::setCubeTexture(VRTexturePtr img, string side, int unit) {
    auto md = mats[activePass];
    // TODO: check for textureobj chunk and remove it

    if (!md->envChunks.count(unit)) { md->envChunks[unit] = TextureEnvChunk::create(); md->regChunk(md->envChunks[unit], unit, 17); }
    md->envChunks[unit]->setEnvMode(GL_MODULATE);

    if (!md->ctxChunks.count(unit)) { md->ctxChunks[unit] = CubeTextureObjChunk::create(); md->regChunk(md->ctxChunks[unit], unit, 16); }
    if (side == "front")  md->ctxChunks[unit]->setImage    (img->getImage());
    if (side == "back")   md->ctxChunks[unit]->setPosZImage(img->getImage());
    if (side == "left")   md->ctxChunks[unit]->setNegXImage(img->getImage());
    if (side == "right")  md->ctxChunks[unit]->setPosXImage(img->getImage());
    if (side == "top")    md->ctxChunks[unit]->setPosYImage(img->getImage());
    if (side == "bottom") md->ctxChunks[unit]->setNegYImage(img->getImage());
}

void VRMaterial::setTextureType(string type, int unit) {
    auto md = mats[activePass];
    if (type == "Normal") {
        if (!md->genChunks.count(unit)) return;
        md->mat->subChunk(md->genChunks[unit]);
        md->genChunks.erase(unit);
        return;
    }

    if (type == "SphereEnv") {
        if (!md->genChunks.count(unit)) { md->genChunks[unit] = TexGenChunk::create(); md->regChunk(md->genChunks[unit], unit, 15); }
        md->genChunks[unit]->setGenFuncS(GL_SPHERE_MAP);
        md->genChunks[unit]->setGenFuncT(GL_SPHERE_MAP);
    }

    if (type == "CubeEnv") {
        if (!md->genChunks.count(unit)) { md->genChunks[unit] = TexGenChunk::create(); md->regChunk(md->genChunks[unit], unit, 14); }
        md->genChunks[unit]->setGenFuncS(GL_REFLECTION_MAP); // GL_REFLECTION_MAP
        md->genChunks[unit]->setGenFuncT(GL_REFLECTION_MAP);
        md->genChunks[unit]->setGenFuncR(GL_REFLECTION_MAP);

        if (!md->tnsChunks.count(unit)) { md->tnsChunks[unit] = TextureTransformChunk::create(); md->regChunk(md->tnsChunks[unit], unit, 13); }
        md->tnsChunks[unit]->setUseCameraBeacon(true);
    }
}

void VRMaterial::setQRCode(string s, Vec3d fg, Vec3d bg, int offset) {
#ifndef WITHOUT_QRENCODE
    createQRCode(s, ptr(), fg, bg, offset);
    setTextureParams(GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, -1, -1, -1);
#endif
}

void VRMaterial::setZOffset(float factor, float bias) {
    auto md = mats[activePass];
    if (md->polygonChunk == 0) { md->polygonChunk = PolygonChunk::create(); md->regChunk(md->polygonChunk, -2, 12); }
    md->polygonChunk->setOffsetFactor(factor);
    md->polygonChunk->setOffsetBias(bias);
    md->polygonChunk->setOffsetFill(true);
}

// higher key means rendered later, for example 0 is rendered first, 1 second
void VRMaterial::setSortKey(int key) {
    auto md = mats[activePass];
    passes->mat->setSortKey(key);
}

void VRMaterial::setFrontBackModes(int front, int back) {
    auto md = mats[activePass];
    if (md->polygonChunk == 0) { md->polygonChunk = PolygonChunk::create(); md->regChunk(md->polygonChunk, -2, 11); }

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

        if (active) md->regChunk(md->clipChunk, -2, 10);
        else md->mat->subChunk(md->clipChunk);

        md->clipChunk->setEquation(Vec4f(equation));
        md->clipChunk->setEnable  (active);
        if (beacon) md->clipChunk->setBeacon(beacon->getNode()->node);
    }
}

void VRMaterial::setWireFrame(bool b) {
#ifndef OSG_OGL_ES2
    if (b) setFrontBackModes(GL_LINE, GL_LINE);
    else setFrontBackModes(GL_FILL, GL_FILL);
#endif
}

bool VRMaterial::isWireFrame() {
#ifndef OSG_OGL_ES2
    auto md = mats[activePass];
    if (md->polygonChunk == 0) return false;
    return bool(md->polygonChunk->getFrontMode() == GL_LINE && md->polygonChunk->getBackMode() == GL_LINE);
#else
    return false;
#endif
}

VRVideoPtr VRMaterial::setVideo(string vid_path) {
    auto md = mats[activePass];
#ifndef WITHOUT_AV
    if (md->video == 0) md->video = VRVideo::create( ptr() );
    md->video->open(vid_path);
#endif
    return md->video;
}

VRVideoPtr VRMaterial::getVideo() { return mats[activePass]->video; }

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
                for (unsigned int i=0; i<cmat->getMFChunks()->size(); i++) {
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
                for (unsigned int i=0; i<cmat->getMFChunks()->size(); i++) {
                    StateChunkMTRecPtr chunk = cmat->getChunk(i);
                    bchunk = dynamic_pointer_cast<BlendChunk>(chunk);
                    if (bchunk) return bchunk;
                }
            }
            return bchunk;
        }
};

namespace OSG {
Color4f toColor4f(Color3f c, float t) { return Color4f(c[0], c[1], c[2], t); }
Color3f toColor3f(Color4f c) { return Color3f(c[0], c[1], c[2]); }
}


void VRMaterial::setTransparency(float c) {
    recUndo(&VRMaterial::setTransparency, ptr(), getTransparency(), c);
    auto md = mats[activePass];
    md->colChunk->setDiffuse( toColor4f(getDiffuse(), c) );

    if (c == 1 || deferred) clearTransparency();
    else enableTransparency();
    //enableTransparency();
}

bool VRMaterial::doesIgnoreMeshColors() {
    auto md = mats[activePass];
    return (md->colChunk->getColorMaterial() == GL_NONE);
}

void VRMaterial::ignoreMeshColors(bool b) {
    auto md = mats[activePass];
    if (b) {
        md->colChunk->setColorMaterial(GL_NONE);
        md->colChunk->setBackColorMaterial(GL_NONE);
    } else {
        md->colChunk->setColorMaterial(GL_DIFFUSE);
        md->colChunk->setBackColorMaterial(GL_DIFFUSE);
    }
}

void VRMaterial::enableTransparency(bool user_override) {
    if (deferred) return;
    force_transparency = user_override;
    auto md = mats[activePass];
    if (md->blendChunk == 0) {
        md->blendChunk = BlendChunk::create();
        md->regChunk(md->blendChunk, -2, 6);
    }
    md->blendChunk->setSrcFactor  ( GL_SRC_ALPHA           );
    md->blendChunk->setDestFactor ( GL_ONE_MINUS_SRC_ALPHA );
}

void VRMaterial::clearTransparency(bool user_override) {
    if (user_override) force_transparency = false;
    if (force_transparency && !user_override && !deferred) return;
    auto md = mats[activePass];
    md->clearChunk(md->blendChunk);
    md->colChunk->setDiffuse(toColor4f(getDiffuse(), 1.0));
    //md->clearChunk(md->depthChunk); // messes with depth tests ?
}

void VRMaterial::setDepthTest(int d) {
    auto md = mats[activePass];
    if (md->depthChunk == 0) {
        md->depthChunk = DepthChunk::create();
        md->regChunk(md->depthChunk, -2, 8);
    }
    md->depthChunk->setFunc(d); // GL_ALWAYS
}

int VRMaterial::getDepthTest() {
    auto md = mats[activePass];
    if (md->depthChunk == 0) return 0;
    return md->depthChunk->getFunc();
}

void VRMaterial::setDiffuse(string c) { setDiffuse(toColor(c)); }
void VRMaterial::setSpecular(string c) { setSpecular(toColor(c)); }
void VRMaterial::setAmbient(string c) { setAmbient(toColor(c)); }
void VRMaterial::setDiffuse(Color3f c) { mats[activePass]->colChunk->setDiffuse( toColor4f(c, getTransparency()) ); updateOGL2Parameters(); }
void VRMaterial::setSpecular(Color3f c) { mats[activePass]->colChunk->setSpecular(toColor4f(c)); updateOGL2Parameters(); }
void VRMaterial::setAmbient(Color3f c) { mats[activePass]->colChunk->setAmbient(toColor4f(c)); updateOGL2Parameters(); }
void VRMaterial::setEmission(Color3f c) { mats[activePass]->colChunk->setEmission(toColor4f(c)); updateOGL2Parameters(); }
void VRMaterial::setShininess(float c) { mats[activePass]->colChunk->setShininess(c); updateOGL2Parameters(); }

void VRMaterial::setLit(bool b) {
    if (activePass >= 0 && activePass < (int)mats.size() && mats[activePass]->colChunk) {
        mats[activePass]->colChunk->setLit(b);
        updateDeferredShader();
        updateOGL2Parameters();
    }
}

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

void regVProgramVars(ShaderProgram* vp) {
    ShaderProgramVariables* vars = vp->getVariables();
    const MFUnrecShaderValueVariablePtr* mfvvars = vars->getMFVariables();
    const MFUnrecChildShaderProcVariablePtr* mfpvars = vars->getMFProceduralVariables();
    for (size_t i=0; i < mfvvars->size(); i++) VRMaterial::fieldContainerMap[(*mfvvars)[i]->getId()] = vp->getId();
    for (size_t i=0; i < mfpvars->size(); i++) VRMaterial::fieldContainerMap[(*mfpvars)[i]->getId()] = vp->getId();
}

void VRMaterial::initShaderChunk() {
    auto md = mats[activePass];
    if (md->shaderChunk != 0) return;
#ifdef OSG_OGL_ES2
	md->shaderFailChunk = ShaderProgramChunk::create();
	ShaderProgramRefPtr vFProgram = ShaderProgram::createVertexShader  ();
	ShaderProgramRefPtr fFProgram = ShaderProgram::createFragmentShader();
	vFProgram->createDefaulAttribMapping();
	vFProgram->addOSGVariable("OSGModelViewProjectionMatrix");
	vFProgram->setProgram(vertFailShader);
	fFProgram->setProgram(fragFailShader);
	fFProgram->addOSGVariable("OSGActiveLightsMask");
	md->shaderFailChunk->addShader(vFProgram);
	md->shaderFailChunk->addShader(fFProgram);
#endif

    md->shaderChunk = ShaderProgramChunk::create();
    md->regChunk(md->shaderChunk, -2, 7);
    auto scID = md->shaderChunk->getId();

    md->vProgram = ShaderProgram::createVertexShader  ();
    md->fProgram = ShaderProgram::createFragmentShader();
    md->fdProgram = ShaderProgram::createFragmentShader();
    md->gProgram = ShaderProgram::createGeometryShader();

    md->tcProgram = ShaderProgram::create();
    md->tcProgram->setShaderType(GL_TESS_CONTROL_SHADER);
    md->teProgram = ShaderProgram::create();
    md->teProgram->setShaderType(GL_TESS_EVALUATION_SHADER);

    // link shaderprogramchunk to is programs
    fieldContainerMap[md->vProgram->getId()] = scID;
    fieldContainerMap[md->fProgram->getId()] = scID;
    fieldContainerMap[md->fdProgram->getId()] = scID;
    fieldContainerMap[md->gProgram->getId()] = scID;
    fieldContainerMap[md->tcProgram->getId()] = scID;
    fieldContainerMap[md->teProgram->getId()] = scID;

    md->shaderChunk->addShader(md->vProgram);

    if (md->deferred) md->shaderChunk->addShader(md->fdProgram);
    else              md->shaderChunk->addShader(md->fProgram);

    md->shaderChunk->addShader(md->gProgram);
    md->shaderChunk->addShader(md->tcProgram);
    md->shaderChunk->addShader(md->teProgram);

    md->vProgram->createDefaulAttribMapping();
    md->vProgram->addOSGVariable("OSGViewportSize");
	md->vProgram->addOSGVariable("OSGNormalMatrix");
	md->vProgram->addOSGVariable("OSGModelViewProjectionMatrix");
	regVProgramVars(md->vProgram);
}

void VRMaterial::enableShaderParameter(string name) {
    auto md = mats[activePass];
    md->vProgram->addOSGVariable(name.c_str());
	regVProgramVars(md->vProgram);
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

// type: GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, ...
bool VRMaterial::checkShader(int type, string shader, string name) {
#ifndef OSG_OGL_ES2
#ifndef WITHOUT_GTK
    auto gm = VRGuiManager::get(false);
    if (!gm) return true;
	auto errC = gm->getConsole("Errors");
	if (!errC) return true;
#endif
#ifndef _WIN32
    if (!glXGetCurrentContext()) return true;

    GLuint shaderObject = glCreateShader(type);
    int N = shader.size();
    const char* str = shader.c_str();
    glShaderSourceARB(shaderObject, 1, &str, &N);
    glCompileShaderARB(shaderObject);

    GLint compiled;
    glGetObjectParameterivARB(shaderObject, GL_COMPILE_STATUS, &compiled);
#ifndef WITHOUT_GTK
    if (!compiled) errC->write( "Shader "+name+" of material "+getName()+" did not compile!\n");
#else
	if (!compiled) cout << "Shader " + name + " of material " + getName() + " did not compile!\n" << endl;
#endif

    GLint blen = 0;
    GLsizei slen = 0;
    glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH , &blen);
    if (blen > 1) {
        GLchar* compiler_log = (GLchar*)malloc(blen);
        glGetInfoLogARB(shaderObject, blen, &slen, compiler_log);
#ifndef WITHOUT_GTK
        errC->write( "Shader "+name+" of material "+getName()+" warnings and errors:\n");
        errC->write( string(compiler_log));
#else
		cout << "Shader " + name + " of material " + getName() + " warnings and errors:\n" << endl;
		cout << string(compiler_log) << endl;
#endif
        free(compiler_log);
	}
#endif
#else
    GLuint shaderObject = glCreateShader(type);
    int N = shader.size();
    const char* str = shader.c_str();
    glShaderSource(shaderObject, 1, &str, &N);
    glCompileShader(shaderObject);

    GLint compiled;
    glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &compiled);
    if (!compiled) cout << "Shader "+name+" of material "+getName()+" did not compiled!\n";

    GLint blen = 0;
    GLsizei slen = 0;
    glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH , &blen);
    if (blen > 1) {
        GLchar* compiler_log = (GLchar*)malloc(blen);
        glGetShaderInfoLog(shaderObject, blen, &slen, compiler_log);
        cout << "Shader "+name+" of material "+getName()+" warnings and errors:\n";
        cout << string(compiler_log);
        free(compiler_log);
        return false;
    }
#endif
    return true;
}

void VRMaterial::forceShaderUpdate() {
    string s = mats[activePass]->vProgram->getProgram();
    mats[activePass]->vProgram->setProgram(s.c_str());
}

void VRMaterial::setVertexShader(string s, string name) {
    initShaderChunk();
    auto m = mats[activePass];

#ifdef WASM
    s = "#define WEBGL\n" + s;
#endif

#ifndef OSG_OGL_ES2
    m->vProgram->setProgram(s);
    checkShader(GL_VERTEX_SHADER, s, name);
    m->tmpDeferredShdr = false;
#else
	m->vertShaderFail = !checkShader(GL_VERTEX_SHADER, s, name);
	if (m->vertShaderFail) {
		if (m->mat->find(m->shaderChunk) != -1) {
			m->mat->subChunk(m->shaderChunk);
			m->mat->addChunk(m->shaderFailChunk);
		}
	} else {
		m->vProgram->setProgram(s);
		if (!m->fragShaderFail && m->mat->find(m->shaderFailChunk) != -1) {
			m->mat->subChunk(m->shaderFailChunk);
			m->mat->addChunk(m->shaderChunk);
		}
	}
    m->tmpOGLESShdr = false;
#endif
}

void VRMaterial::setFragmentShader(string s, string name, bool deferred) {
    initShaderChunk();
    auto m = mats[activePass];
#ifdef WASM
    if (!contains(s, "precision mediump"))
        s = "#define WEBGL\nprecision mediump float;\n" + s;
#endif

#ifndef OSG_OGL_ES2
    if (deferred) m->fdProgram->setProgram(s.c_str());
    else          m->fProgram->setProgram(s.c_str());
    checkShader(GL_FRAGMENT_SHADER, s, name);
    m->tmpDeferredShdr = false;
#else
	m->fragShaderFail = !checkShader(GL_FRAGMENT_SHADER, s, name);
	if (m->fragShaderFail) {
		if (m->mat->find(m->shaderChunk) != -1) {
			m->mat->subChunk(m->shaderChunk);
			m->mat->addChunk(m->shaderFailChunk);
		}
	} else {
		m->fProgram->setProgram(s);
		if (!m->vertShaderFail && m->mat->find(m->shaderFailChunk) != -1) {
			m->mat->subChunk(m->shaderFailChunk);
			m->mat->addChunk(m->shaderChunk);
		}
	}
    m->tmpOGLESShdr = false;
#endif
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

void VRMaterial::readVertexShader(string s) { if (exists(s)) setVertexShader(readFile(s), s); }
void VRMaterial::readFragmentShader(string s, bool deferred) { if (exists(s)) setFragmentShader(readFile(s), s, deferred); }
void VRMaterial::readGeometryShader(string s) { if (exists(s)) setGeometryShader(readFile(s), s); }
void VRMaterial::readTessControlShader(string s) { if (exists(s)) setTessControlShader(readFile(s), s); }
void VRMaterial::readTessEvaluationShader(string s) { if (exists(s)) setTessEvaluationShader(readFile(s), s); }

string VRMaterial::getVertexShader() {
    auto m = mats[activePass];
    if (m->vProgram) return m->vProgram->getProgram();
    return "";
}

string VRMaterial::getFragmentShader(bool deferred) {
    auto m = mats[activePass];
    if (deferred && m->fdProgram) return m->fdProgram->getProgram();
    if (!deferred && m->fProgram) return m->fProgram->getProgram();
    return "";
}

string VRMaterial::getGeometryShader() {
    auto m = mats[activePass];
    if (m->gProgram) return m->gProgram->getProgram();
    return "";
}

string VRMaterial::getTessControlShader() {
    auto m = mats[activePass];
    if (m->tcProgram) return m->tcProgram->getProgram();
    return "";
}

string VRMaterial::getTessEvaluationShader() {
    auto m = mats[activePass];
    if (m->teProgram) return m->teProgram->getProgram();
    return "";
}

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

    auto conv = [](char c1, char c2) {
        if (c1 >= 'A' && c1 <= 'F') c1 -= ('A'-'a');
        if (c2 >= 'A' && c2 <= 'F') c2 -= ('A'-'a');
        int C1 = c1-'0';
        int C2 = c2-'0';
        if (c1 >= 'a' && c1 <= 'f') C1 = (c1-'a')+10;
        if (c2 >= 'a' && c2 <= 'f') C2 = (c2-'a')+10;
        return (C1*16+C2)/256.0;
    };

    if (c[0] == '#') {
        if (c.size() == 4) return Color3f(conv(c[1],'a'), conv(c[2],'a'), conv(c[3],'a'));
        if (c.size() == 7) return Color3f(conv(c[1],c[2]), conv(c[3],c[4]), conv(c[5],c[6]));
        return Color3f();
    }

    return colorMap.count(c) ? colorMap[c] : Color3f();
}

string VRMaterial::diffPass(VRMaterialPtr m, int pass) {
    string res;
    auto& p1 = mats[pass];
    auto& p2 = m->mats[pass];

    auto either = [](bool a, bool b) { return ((a && !b) || (!a && b)); };

    if (either(p1->mat, p2->mat)) res += "\ndiff on mat|"+toString(bool(p1->mat))+"|"+toString(bool(p2->mat));
    if (either(p1->colChunk, p2->colChunk)) res += "\ndiff on colChunk|"+toString(bool(p1->colChunk))+"|"+toString(bool(p2->colChunk));
    if (either(p1->blendChunk, p2->blendChunk)) res += "\ndiff on blendChunk|"+toString(bool(p1->blendChunk))+"|"+toString(bool(p2->blendChunk));
    if (either(p1->depthChunk, p2->depthChunk)) res += "\ndiff on depthChunk|"+toString(bool(p1->depthChunk))+"|"+toString(bool(p2->depthChunk));
    if (either(p1->lineChunk, p2->lineChunk)) res += "\ndiff on lineChunk|"+toString(bool(p1->lineChunk))+"|"+toString(bool(p2->lineChunk));
    if (either(p1->pointChunk, p2->pointChunk)) res += "\ndiff on pointChunk|"+toString(bool(p1->pointChunk))+"|"+toString(bool(p2->pointChunk));
    if (either(p1->polygonChunk, p2->polygonChunk)) res += "\ndiff on polygonChunk|"+toString(bool(p1->polygonChunk))+"|"+toString(bool(p2->polygonChunk));
    if (either(p1->twoSidedChunk, p2->twoSidedChunk)) res += "\ndiff on twoSidedChunk|"+toString(bool(p1->twoSidedChunk))+"|"+toString(bool(p2->twoSidedChunk));
    if (either(p1->shaderChunk, p2->shaderChunk)) res += "\ndiff on shaderChunk|"+toString(bool(p1->shaderChunk))+"|"+toString(bool(p2->shaderChunk));
    if (either(p1->clipChunk, p2->clipChunk)) res += "\ndiff on clipChunk|"+toString(bool(p1->clipChunk))+"|"+toString(bool(p2->clipChunk));
    if (either(p1->stencilChunk, p2->stencilChunk)) res += "\ndiff on stencilChunk|"+toString(bool(p1->stencilChunk))+"|"+toString(bool(p2->stencilChunk));

    if (either(p1->vProgram, p2->vProgram)) res += "\ndiff on vProgram|"+toString(bool(p1->vProgram))+"|"+toString(bool(p2->vProgram));
    if (either(p1->fProgram, p2->fProgram)) res += "\ndiff on fProgram|"+toString(bool(p1->fProgram))+"|"+toString(bool(p2->fProgram));
    if (either(p1->fdProgram, p2->fdProgram)) res += "\ndiff on fdProgram|"+toString(bool(p1->fdProgram))+"|"+toString(bool(p2->fdProgram));
    if (either(p1->gProgram, p2->gProgram)) res += "\ndiff on gProgram|"+toString(bool(p1->gProgram))+"|"+toString(bool(p2->gProgram));
    if (either(p1->tcProgram, p2->tcProgram)) res += "\ndiff on tcProgram|"+toString(bool(p1->tcProgram))+"|"+toString(bool(p2->tcProgram));
    if (either(p1->teProgram, p2->teProgram)) res += "\ndiff on teProgram|"+toString(bool(p1->teProgram))+"|"+toString(bool(p2->teProgram));

    if (either(bool(p1->video), bool(p2->video))) res += "\ndiff on video|"+toString(bool(p1->video))+"|"+toString(bool(p2->video));
    if (either(p1->deferred, p2->deferred)) res += "\ndiff on deferred|"+toString(bool(p1->deferred))+"|"+toString(bool(p2->deferred));
    if (either(p1->tmpDeferredShdr, p2->tmpDeferredShdr)) res += "\ndiff on tmpDeferredShdr|"+toString(bool(p1->tmpDeferredShdr))+"|"+toString(bool(p2->tmpDeferredShdr));
    if (either(p1->tmpOGLESShdr, p2->tmpOGLESShdr)) res += "\ndiff on tmpOGLESShdr|"+toString(bool(p1->tmpOGLESShdr))+"|"+toString(bool(p2->tmpOGLESShdr));

    if (p1->vertexScript != p2->vertexScript) res += "\ndiff on vertexScript|"+p1->vertexScript+"|"+p2->vertexScript;
    if (p1->fragmentScript != p2->fragmentScript) res += "\ndiff on fragmentScript|"+p1->fragmentScript+"|"+p2->fragmentScript;
    if (p1->fragmentDScript != p2->fragmentDScript) res += "\ndiff on fragmentDScript|"+p1->fragmentDScript+"|"+p2->fragmentDScript;
    if (p1->geometryScript != p2->geometryScript) res += "\ndiff on geometryScript|"+p1->geometryScript+"|"+p2->geometryScript;
    if (p1->tessControlScript != p2->tessControlScript) res += "\ndiff on tessControlScript|"+p1->tessControlScript+"|"+p2->tessControlScript;
    if (p1->tessEvalScript != p2->tessEvalScript) res += "\ndiff on tessEvalScript|"+p1->tessEvalScript+"|"+p2->tessEvalScript;

    if (p1->texChunks.size() != p2->texChunks.size()) res += "\ndiff on N texChunks|"+toString(p1->texChunks.size())+"|"+toString(p2->texChunks.size());
    if (p1->envChunks.size() != p2->envChunks.size()) res += "\ndiff on N envChunks|"+toString(p1->envChunks.size())+"|"+toString(p2->envChunks.size());
    if (p1->ctxChunks.size() != p2->ctxChunks.size()) res += "\ndiff on N ctxChunks|"+toString(p1->ctxChunks.size())+"|"+toString(p2->ctxChunks.size());
    if (p1->genChunks.size() != p2->genChunks.size()) res += "\ndiff on N genChunks|"+toString(p1->genChunks.size())+"|"+toString(p2->genChunks.size());
    if (p1->tnsChunks.size() != p2->tnsChunks.size()) res += "\ndiff on N tnsChunks|"+toString(p1->tnsChunks.size())+"|"+toString(p2->tnsChunks.size());

    return res;
}

string VRMaterial::diff(VRMaterialPtr m) {
    string res;
    if (mats.size() != m->mats.size()) res += "\nN passes|"+toString(mats.size())+"|"+toString(m->mats.size());
    else {
        for (unsigned int i=0; i<mats.size(); i++) res += diffPass(m, i);
    }
    return res;
}
