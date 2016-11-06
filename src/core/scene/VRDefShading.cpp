#include "VRDefShading.h"

#include <OpenSG/OSGDeferredShadingStage.h>
#include <OpenSG/OSGDirectionalLight.h>
#include <OpenSG/OSGPointLight.h>
#include <OpenSG/OSGSpotLight.h>
#include <OpenSG/OSGImage.h>

#include <OpenSG/OSGImageForeground.h>
#include <OpenSG/OSGPolygonForeground.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGTextureObjChunk.h>

#include <OpenSG/OSGShaderProgramChunk.h>
#include <OpenSG/OSGShaderProgram.h>
#include <OpenSG/OSGShaderShadowMapEngine.h>
#include <OpenSG/OSGTrapezoidalShadowMapEngine.h>

#include <OpenSG/OSGFrameBufferObject.h>
#include <OpenSG/OSGTextureBuffer.h>
#include <OpenSG/OSGRenderBuffer.h>

#include "core/utils/VROptions.h"
#include "core/objects/object/OSGCore.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRCamera.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/scene/VRSceneManager.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRDefShading::VRDefShading() {
    cout << "VRDefShading\n";
    defaultShadowType = ST_TRAPEZOID;
    shadowRes = 1024;
    shadowColor = 0.3;
}

VRDefShading::~VRDefShading() {
    cout << "~VRDefShading\n";
}

void VRDefShading::init() {
    shadowMapWidth  = shadowRes;
    shadowMapHeight = shadowRes;

    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/DeferredShading/";
    dsGBufferVPFile = resDir + "DSGBuffer.vp.glsl";
    dsGBufferFPFile = resDir + "DSGBuffer.fp.glsl";
    dsAmbientVPFile = resDir + "DSAmbient.vp.glsl";
    dsAmbientFPFile = resDir + "DSAmbient.fp.glsl";
    dsDirLightVPFile = resDir + "DSDirLight.vp.glsl";
    dsDirLightFPFile = resDir + "DSDirLight.fp.glsl";
    dsDirLightShadowFPFile = resDir + "DSDirLightShadow.fp.glsl";
    dsPointLightVPFile = resDir + "DSPointLight.vp.glsl";
    dsPointLightFPFile = resDir + "DSPointLight.fp.glsl";
    dsPointLightShadowFPFile = resDir + "DSPointLightShadow.fp.glsl";
    dsSpotLightVPFile = resDir + "DSSpotLight.vp.glsl";
    dsSpotLightFPFile = resDir + "DSSpotLight.fp.glsl";
    dsSpotLightShadowFPFile = resDir + "DSSpotLightShadow.fp.glsl";

    dsStage  = DeferredShadingStage::create();
    dsStage->editMFPixelFormats()->push_back(Image::OSG_RGBA_PF          ); // positions (RGB) + ambient (A) term buffer
    dsStage->editMFPixelTypes  ()->push_back(Image::OSG_FLOAT32_IMAGEDATA);
    dsStage->editMFPixelFormats()->push_back(Image::OSG_RGB_PF           ); // normals (RGB) buffer
    dsStage->editMFPixelTypes  ()->push_back(Image::OSG_FLOAT32_IMAGEDATA);
    dsStage->editMFPixelFormats()->push_back(Image::OSG_RGB_PF); // diffuse (RGB) buffer
    dsStage->editMFPixelTypes  ()->push_back(Image::OSG_UINT8_IMAGEDATA);

    //dsStage->editMFPixelFormats()->push_back(Image::OSG_RGB_PF); // diffuse2 (RGB) buffer
    //dsStage->editMFPixelTypes  ()->push_back(Image::OSG_UINT8_IMAGEDATA);

    dsStage->setGBufferProgram(NULL);

    // ambient shader
    ShaderProgramRecPtr      vpAmbient = ShaderProgram::createVertexShader  ();
    ShaderProgramRecPtr      fpAmbient = ShaderProgram::createFragmentShader();
    ShaderProgramChunkRecPtr shAmbient = ShaderProgramChunk::create();
    vpAmbient->readProgram(dsAmbientVPFile.c_str());
    fpAmbient->readProgram(dsAmbientFPFile.c_str());
    fpAmbient->addUniformVariable<Int32>("texBufNorm", 1);
    shAmbient->addShader(vpAmbient);
    shAmbient->addShader(fpAmbient);
    dsStage->setAmbientProgram(shAmbient);

    // fbo -> TODO
    /*FrameBufferObjectRefPtr fbo = FrameBufferObject::create();
    ImageRecPtr img = Image::create();
    tex = TextureObjChunk::create();
    TextureBufferRefPtr texBuf = TextureBuffer::create();
    RenderBufferRefPtr depthBuf = RenderBuffer::create();

    fbo->editMFDrawBuffers()->push_back(GL_COLOR_ATTACHMENT0_EXT);
    fbo->setPostProcessOnDeactivate(true);
    dsStage->setRenderTarget(fbo);
    tex->setImage(img);
    texBuf->setTexture(tex);
    depthBuf->setInternalFormat(GL_DEPTH_COMPONENT24); // 16 24 32
    fbo->setColorAttachment(texBuf, 0);
    fbo->setDepthAttachment(depthBuf);

    img->set(Image::OSG_RGBA_PF, 1200, 800);
    fbo->setWidth (1200);
    fbo->setHeight(800);*/

    initiated = true;
}

void VRDefShading::reload() {
    ShaderProgramRecPtr      vpAmbient = ShaderProgram::createVertexShader  ();
    ShaderProgramRecPtr      fpAmbient = ShaderProgram::createFragmentShader();
    ShaderProgramChunkRecPtr shAmbient = ShaderProgramChunk::create();
    vpAmbient->readProgram(dsAmbientVPFile.c_str());
    fpAmbient->readProgram(dsAmbientFPFile.c_str());
    fpAmbient->addUniformVariable<Int32>("texBufNorm", 1);
    shAmbient->addShader(vpAmbient);
    shAmbient->addShader(fpAmbient);
    dsStage->setAmbientProgram(shAmbient);

    for (auto li : lightInfos) {
        string vpFile = getLightVPFile(li.second.lightType);
        string fpFile = getLightFPFile(li.second.lightType, li.second.shadowType);
        li.second.lightVP->readProgram(vpFile.c_str());
        li.second.lightFP->readProgram(fpFile.c_str());
        li.second.lightFP->addUniformVariable<Int32>("channel", channel);
    }
}

void VRDefShading::initDeferredShading(VRObjectPtr o) {
    init();
    stageObject = o;
    setDefferedShading(enabled);
}

void VRDefShading::setDefferedShading(bool b) {
    enabled = b;
    if (stageObject == 0) return;
    if (b) stageObject->setCore(OSGCore::create(dsStage), "defShading", true);
    else stageObject->setCore(OSGCore::create(Group::create()), "Object", true);
}

bool VRDefShading::getDefferedShading() { return enabled; }

// channel : GL_RENDER GL_POSITION GL_NORMALIZE GL_DIFFUSE
void VRDefShading::setDeferredChannel(int c) { channel = c; reload(); }

void VRDefShading::setDSCamera(VRCameraPtr cam) {
    if (initiated) dsStage->setCamera(cam->getCam());
}

void VRDefShading::setDSCamera(ProjectionCameraDecoratorRecPtr cam) {
    if (initiated) dsStage->setCamera(cam);
}

void VRDefShading::addDSLight(VRLightPtr vrl) {
    if (!initiated) return;

    LightMTRecPtr light = vrl->getLightCore();
    string type = vrl->getLightType();
    bool shadows = vrl->getShadows();
    int ID = vrl->getID();

    LightInfo li;

    li.lightVP    = ShaderProgram     ::createVertexShader  ();
    li.lightFP    = ShaderProgram     ::createFragmentShader();
    li.lightSH    = ShaderProgramChunk::create              ();

    if (shadows) li.shadowType = defaultShadowType;
    else li.shadowType = ST_NONE;

    li.light = light;
    int t = 1;
    if (type == "directional") t = 2;
    if (type == "spot") t = 3;
    li.lightType = LightTypeE(t);

    li.lightFP->addUniformVariable<Int32>("texBufPos",  0);
    li.lightFP->addUniformVariable<Int32>("texBufNorm", 1);
    li.lightFP->addUniformVariable<Int32>("texBufDiff", 2);
    li.lightFP->addUniformVariable<Int32>("texBufAmb",  3);
    li.lightFP->addUniformVariable<float>("shadowColor", shadowColor);

    li.lightSH->addShader(li.lightVP);
    li.lightSH->addShader(li.lightFP);

    li.dsID = dsStage->editMFLights()->size();
    dsStage->editMFLights       ()->push_back(li.light  );
    dsStage->editMFLightPrograms()->push_back(li.lightSH);
    lightInfos[ID] = li;
    setShadow(li);
}

void VRDefShading::updateLight(VRLightPtr l) {
    auto& li = lightInfos[l->getID()];

    string type = l->getLightType();
    bool shadows = l->getShadows();

    li.lightType = Point;
    if (type == "directional") li.lightType = Directional;
    if (type == "spot") li.lightType = Spot;
    if (shadows) li.shadowType = defaultShadowType;
    else li.shadowType = ST_NONE;

    li.light = l->getLightCore();
    dsStage->editMFLights()->replace(li.dsID, li.light);

    string vpFile = getLightVPFile(li.lightType);
    string fpFile = getLightFPFile(li.lightType, li.shadowType);
    li.lightVP->readProgram(vpFile.c_str());
    li.lightFP->readProgram(fpFile.c_str());
}

TextureObjChunkRefPtr VRDefShading::getTarget() { return fboTex; }

void VRDefShading::setShadow(LightInfo &li) {
    dsStage->editMFLights       ();
    dsStage->editMFLightPrograms();

    string vpFile = getLightVPFile(li.lightType);
    string fpFile = getLightFPFile(li.lightType, li.shadowType);

    li.lightVP->readProgram(vpFile.c_str());
    li.lightFP->readProgram(fpFile.c_str());

    if(li.shadowType == ST_NONE) {
        li.light->setLightEngine(NULL);
        return;
    }

    if(li.shadowType == ST_STANDARD) {
        ShaderShadowMapEngineRecPtr shadowEng = ShaderShadowMapEngine::create();

        shadowEng->setWidth (shadowMapWidth );
        shadowEng->setHeight(shadowMapHeight);
        shadowEng->setOffsetFactor( 4.5f);
        shadowEng->setOffsetBias  (16.f );
        shadowEng->setForceTextureUnit(3);

        li.light->setLightEngine(shadowEng);
        return;
    }

    if(li.shadowType == ST_TRAPEZOID) {
        if(li.lightType != Directional) {
            TrapezoidalShadowMapEngineRecPtr shadowEng =
                TrapezoidalShadowMapEngine::create();

            shadowEng->setWidth (shadowMapWidth );
            shadowEng->setHeight(shadowMapHeight);
            shadowEng->setOffsetFactor( 4.5f);
            shadowEng->setOffsetBias  (16.f );
            shadowEng->setForceTextureUnit(3);


            li.light->setLightEngine(shadowEng);
        } else {
            std::cout << "TSM not supported for diretional lights." << std::endl;
            li.light->setLightEngine(NULL);
        }
    }
}


// file containing vertex shader code for the light type
const std::string& VRDefShading::getLightVPFile(LightTypeE lightType) {
    switch(lightType) {
        case LightEngine::Directional:
            return dsDirLightVPFile;
        case LightEngine::Point:
            return dsPointLightVPFile;
        case LightEngine::Spot:
            return dsSpotLightVPFile;
        default:
            return dsUnknownFile;
    }
}

// file containing fragment shader code for the light type
const std::string& VRDefShading::getLightFPFile(LightTypeE lightType, ShadowTypeE shadowType) {
    switch(lightType) {
        case Directional:
            if(shadowType == ST_NONE) return dsDirLightFPFile;
            else return dsDirLightShadowFPFile;

        case Point:
            if(shadowType == ST_NONE) return dsPointLightFPFile;
            else return dsPointLightShadowFPFile;

        case Spot:
            if(shadowType == ST_NONE) return dsSpotLightFPFile;
            else return dsSpotLightShadowFPFile;

        default:
            return dsUnknownFile;
    }
}

void VRDefShading::subLight(UInt32 lightIdx, int ID) {
    OSG_ASSERT(lightIdx < lightInfos.size());
    OSG_ASSERT(lightIdx < dsStage->getMFLights()->size());
    OSG_ASSERT(lightIdx < dsStage->getMFLightPrograms()->size());

    dsStage->editMFLights()->erase(lightIdx);
    dsStage->editMFLightPrograms()->erase(lightIdx);

    lightInfos.erase(ID);
}

OSG_END_NAMESPACE;
