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

#include "core/utils/VROptions.h"
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

    initiated = true;
}

void VRDefShading::initDeferredShading(VRObjectPtr o) {
    init();
    stageObject = o;
    setDefferedShading(enabled);
}

void VRDefShading::setDefferedShading(bool b) {
    enabled = b;
    if (stageObject == 0) return;
    if (b) stageObject->setCore(dsStage, "defShading", true);
    else stageObject->setCore(Group::create(), "Object", true);
}

bool VRDefShading::getDefferedShading() { return enabled; }

void VRDefShading::setDSCamera(VRCameraPtr cam) {
    if (initiated) dsStage->setCamera(cam->getCam());
}

void VRDefShading::addDSLight(VRLightPtr light) {
    addDSLight(light->getLightCore(), light->getLightType(), light->getShadows());
}

void VRDefShading::addDSLight(LightMTRecPtr light, string type, bool shadows) {
    if (!initiated) return;

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

    dsStage->editMFLights       ()->push_back(li.light  );
    dsStage->editMFLightPrograms()->push_back(li.lightSH);

    lightInfos.push_back(li);

    setShadow(li);
}


void VRDefShading::setShadow(LightInfo &li) {
    dsStage->editMFLights       ();
    dsStage->editMFLightPrograms();

    std::string vpFile = getLightVPFile(li.lightType);
    std::string fpFile = getLightFPFile(li.lightType, li.shadowType);

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
        if(li.lightType != LightEngine::Directional) {
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
        case LightEngine::Directional:
            if(shadowType == ST_NONE) return dsDirLightFPFile;
            else return dsDirLightShadowFPFile;

        case LightEngine::Point:
            if(shadowType == ST_NONE) return dsPointLightFPFile;
            else return dsPointLightShadowFPFile;

        case LightEngine::Spot:
            if(shadowType == ST_NONE) return dsSpotLightFPFile;
            else return dsSpotLightShadowFPFile;

        default:
            return dsUnknownFile;
    }
}

void VRDefShading::subLight(UInt32 lightIdx) {
    OSG_ASSERT(lightIdx < lightInfos.size());
    OSG_ASSERT(lightIdx < dsStage->getMFLights()->size());
    OSG_ASSERT(lightIdx < dsStage->getMFLightPrograms()->size());

    dsStage->editMFLights()->erase(lightIdx);
    dsStage->editMFLightPrograms()->erase(lightIdx);

    lightInfos.erase(lightInfos.begin() + lightIdx);
}

OSG_END_NAMESPACE;
