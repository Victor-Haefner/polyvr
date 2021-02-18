#include "VRRain.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/geometry/VRSky.h"
#include "core/tools/VRTextureRenderer.h"
#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRScene.h"

#include "core/objects/VRCamera.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/VRAnimation.h"

#include <math.h>
#include <random>

using namespace OSG;

template<> string typeName(const VRRain& t) { return "Rain"; }


/*	Python usage example:
	def initRain():
		VR.rain = VR.Rain()
		VR.rain.setScale(False, 3)
		VR.scene.addChild(VR.rain)
		VR.rain.start()

    initRain()
*/
Vec3f VRRain::convertV3dToV3f(Vec3d in) {
    Vec3f out;
    out[0] = (float)in[0];
    out[1] = (float)in[1];
    out[2] = (float)in[2];
    return out;
}

VRRain::VRRain() : VRGeometry("Rain") {
    type = "Rain";
    setupShaderLocations();

    dropColor = Vec3f(0.3,0.3,0.7);
    dropWidth = 0.1;
    dropLength = 0.1;
    dropSpeed = 1;
    dropDensity = 1;

    lastCt2 = Vec3d(0,0,0);
    lastCt3 = Vec3d(0,0,0);
    lastCt5 = Vec3d(0,0,0);
    lastCt8 = Vec3d(0,0,0);

    //Shader setup
    mat = VRMaterial::create("Rain");
    reloadShader();
    setMaterial(mat);
    setPrimitive("Plane 2 2 1 1");
    mat->setLit(false);
	mat->setDiffuse(Color3f(1));
	mat->setShaderParameter<float>("rainOffset", offset);
	mat->setShaderParameter<float>("rainDensity", rainDensity);
	mat->setShaderParameter<float>("camH", camH);
	mat->setShaderParameter<Vec3f>("dropColor", dropColor);
	mat->setShaderParameter<float>("dropWidth", dropWidth);
	mat->setShaderParameter<float>("dropLength", dropLength);
	mat->enableTransparency();

    setVolumeCheck(false, true);

    //TexRenderer setup
    textureSize = 512;

    auto camDef = VRScene::getCurrent()->getActiveCamera();
    oldCamTex = camDef;

    texRenderer = VRTextureRenderer::create("rainTexRenderer");
    texRenderer->setPersistency(0);
    auto scene = VRScene::getCurrent();
    scene->getRoot()->addChild(texRenderer, true, 0);
    auto lightF = scene->getRoot()->find("light");

    camTex = VRCamera::create("camRainTexture");
    camTex-> setPersistency(0);
    lightF->addChild(camTex);
    camTex->setType(1);
    texRenderer->setup(camTex,512,512);
    camTex->setFrom(Vec3d(0,100,0));
    camTex->setAt(Vec3d(0,1,-1));
    texRenderer->addLink(lightF);

    renderMat = texRenderer->getMaterial();
    renderMat->readVertexShader(vScriptTex);
    renderMat->readFragmentShader(fScriptTex);
    renderMat->setShaderParameter<float>("tex0", 0);
    renderMat->setShaderParameter<float>("tex1", 1);

    mat->setTexture(renderMat->getTexture(1));
    mat->setShaderParameter("tex", 1);

    cube = VRGeometry::create("cubeRainTexture");
    cube->setPersistency(0);
	cube->setMaterial(renderMat);
	lightF->addChild(cube);

    camDef->activate();

	density = densityStart;
    speedX = speedStartX;
    speedY = speedStartY;
    color = colorStart;
    light = lightStart;

    updatePtr = VRUpdateCb::create("rain update", bind(&VRRain::update, this));
    VRScene::getCurrent()->addUpdateFkt(updatePtr);

    cout << "VRRain::VRRain()\n";
}
VRRain::~VRRain() {}

VRRainPtr VRRain::create() {
    auto rain = VRRainPtr( new VRRain() );
    rain->hide("SHADOW");
    return rain;
}

VRRainPtr VRRain::ptr() { return static_pointer_cast<VRRain>( shared_from_this() ); }

float VRRain::get() { return scale; }
bool VRRain::getIsRaining() { return isRaining; }
VRTextureRendererPtr VRRain::getRenderer() { return texRenderer; }
VRMaterialPtr VRRain::getTexMat() { return renderMat; }

void VRRain::start() {
    if (!isRaining) {
        if (scale == 0) setScale(false, 3);
        else setScale(false, scale);

        auto rainAnimation = VRAnimation::create("startRain");
        rainAnimationCb = VRAnimCb::create("rain update", bind(&VRRain::startRainCallback, this, _1));
        rainAnimation->setCallback(rainAnimationCb);
        rainAnimation->setDuration(durationTransition);
        rainAnimation->start();
        isRaining = true;
        cout << "VRRain::startRain()\n";
    }
    else cout << "it's already raining" << endl;
}

void VRRain::startRainCallback(float t) {
    auto sky = VRScene::getCurrent()->getSky();
    if (sky) sky->setClouds(densityStart+(density-densityStart)*t, 1e-5, 3000, Vec2d(speedStartX+(speedX-speedStartX)*t, speedStartY+(speedX-speedStartY)*t), Color4f(colorStart-(colorStart-color)*t,colorStart-(colorStart-color)*t,colorStart-(colorStart-color)*t,1));

	//IDEA: MAYBE CHANGE LIGHT SETTINGS OF SCENE
	//lightMain = VRScene::getCurrent()->getRoot()->find("light");
    //float cl = 1-0.4*t;
	//lightMain->setDiffuse(Color4f(cl,cl,cl,0));

	if (tnow != floor(t*10)) updateScale(scale*floor(t*10)/10);
	tnow = floor(t*10);
}

void VRRain::stop() {
    if (isRaining) {
        auto rainAnimation = VRAnimation::create("stopRain");
        rainAnimationCb = VRAnimCb::create("rain update", bind(&VRRain::stopRainCallback, this, _1));
        rainAnimation->setCallback(rainAnimationCb);
        rainAnimation->setDuration(durationTransition);
        rainAnimation->start();
        isRaining = false;
    }
    else cout << "currently not raining" << endl;
}

void VRRain::stopRainCallback(float t) {
    auto sky = VRScene::getCurrent()->getSky();
    if (sky) sky->setClouds(density-(density-densityStart)*t, 1e-5, 3000, Vec2d(speedX-(speedX-speedStartX)*t, speedX-(speedX-speedStartY)*t), Color4f(color+(colorStart-color)*t,color+(colorStart-color)*t,color+(colorStart-color)*t,1));

    //lightMain = VRScene::getCurrent()->getRoot()->find("light");
    //float cl = 0.6+0.4*t;
	//lightMain->setDiffuse(Color4f(cl,cl,cl,0));
	if (tnow != floor(t*10)) updateScale(scale-scale*floor(t*10)/10);
    tnow = floor(t*10);
}

void VRRain::setScale( bool liveChange, float scale){
    if ( scale<0 || scale>10 ) {
        //TODO: error
        cout << "Input for Scale of Rain should be between 0 and 10" << endl;
    }
    else {
        this->scale = scale;
        density = 0.4+scale*0.06;    //density of clouds
        speedX = 0.004+scale*0.0001;
        speedY = 0.004+scale*0.0001;
        color = 0.5-scale*0.02;
        light = 0.4;
        if (liveChange) {
            updateScale(scale);
            auto sky = VRScene::getCurrent()->getSky();
            if (sky) sky->setClouds(density, 1e-5, 3000, Vec2d(speedX, speedY), Color4f(color,color,color,1));
        }
    }
}

void VRRain::overrideParameters( float durationTransition, float rainDensity, float density, float speedX, float speedY, float color, float light ) {
    this->durationTransition = durationTransition;
    this->rainDensity = rainDensity;
    this->density = density;
    this->speedX = speedX;
    this->speedY = speedY;
    this->color = color;
    this->light = light;
    mat->setShaderParameter<float>("rainDensity", rainDensity);
}

void VRRain::updateScale( float scaleNow ){
    scaleRN = scaleNow;
    float rainDensity = scaleRN == 0 ? 0 : 0.2 * 10/scaleRN;
    mat->setShaderParameter<float>("rainDensity", rainDensity);
    //cout << " rain density: " << rainDensity << endl;
}

void VRRain::setupShaderLocations() {
    string resDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/Rain/";
    vScript = resDir + "Rain.vp";
    fScript = resDir + "Rain.fp";
    dfScript = resDir + "Rain.dfp";
    vScriptTex = resDir + "RainTex.vp";
    fScriptTex = resDir + "RainTex.fp";
    dfScriptTex = resDir + "RainTex.dfp";
}

void VRRain::update() {
    if (!isVisible()) return;

    double offset = getTime()*1e-6; //seconds

    auto camDef = VRScene::getCurrent()->getActiveCamera();
    if (debugRain) cout << "defcam: " << camDef->getName() << endl;
    if (camDef == camTex) {
        camDef = oldCamTex;
        depthTexer = true;
    }
    else {
        oldCamTex = camDef;
        depthTexer = false;
    }
    //if (camDef->getName() == "car") oldCamTex = camDef;
    if (debugRain) cout << "oldcam: " << oldCamTex->getName() << endl;
    auto defCamPos = camDef->getWorldPosition();//camDef->getFrom();
    camTex->setFrom(defCamPos);
    camTex->translate(Vec3d(0,40,0));
    camTex->setAt(defCamPos);
    camTex->setUp(Vec3d(0,0,1));
    if (debugRain) cout << "camdeffrom: " << camDef->getFrom() << endl;
    if (debugRain) cout << "camtexfrom: " << camTex->getFrom() << endl;

    mat->setShaderParameter<float>("rainOffset", offset);
    mat->setShaderParameter<bool>("depthTexer", depthTexer);
	mat->setShaderParameter<Vec3f>("dropColor", dropColor);
	mat->setShaderParameter<float>("dropLength", dropLength);
	mat->setShaderParameter<float>("dropWidth", dropWidth);
	mat->setShaderParameter<float>("dropSpeed", dropSpeed);
	mat->setShaderParameter<float>("dropDensity", dropDensity);
    reloadShader();

    Vec3d tmpPos = Vec3d(defCamPos[0],0,defCamPos[2]);
    if ( (tmpPos-lastCt2).length() > 0.1* 2 ) lastCt2 = tmpPos;
    if ( (tmpPos-lastCt3).length() > 0.3* 3 ) lastCt3 = tmpPos;
    if ( (tmpPos-lastCt5).length() > 0.5* 5 ) lastCt5 = tmpPos;
    if ( (tmpPos-lastCt8).length() > 0.5* 8 ) lastCt8 = tmpPos;

	mat->setShaderParameter<Vec3f>("lastCt2", convertV3dToV3f(lastCt2));
	mat->setShaderParameter<Vec3f>("lastCt3", convertV3dToV3f(lastCt3));
	mat->setShaderParameter<Vec3f>("lastCt5", convertV3dToV3f(lastCt5));
	mat->setShaderParameter<Vec3f>("lastCt8", convertV3dToV3f(lastCt8));

	//cout << toString(lastCt2) << " " << toString(lastCt3) << " " << toString(lastCt5) << " " << toString(lastCt8) << endl;
}

void VRRain::setDropColor(Vec3d clIn) {
    dropColor = convertV3dToV3f(clIn);
}

void VRRain::setDropSize(float dropWidth, float dropLength) {
    this->dropWidth = dropWidth;
    this->dropLength = dropLength;
}

void VRRain::setDropSpeed(float dropSpeed) {
    this->dropSpeed = dropSpeed;
}

void VRRain::setDropDensity(float dropDensity) {
    this->dropDensity = dropDensity;
}

void VRRain::doTestFunction() {
    auto tmp = camTex->getFrom();
    cout << tmp << endl;
}

void VRRain::reloadShader() {
    mat->readVertexShader(vScript);
    mat->readFragmentShader(fScript);
    mat->readFragmentShader(dfScript, true);
    mat->updateDeferredShader();
}

