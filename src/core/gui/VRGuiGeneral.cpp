#include <OpenSG/OSGRenderAction.h>
#include <OpenSG/OSGSceneFileHandler.h>

#include "VRGuiGeneral.h"
#include "VRGuiManager.h"
//#include "VRGuiUtils.h"
//#include "VRGuiFile.h"

#include "core/scene/VRScene.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRView.h"
#include "core/scene/rendering/VRRenderStudio.h"
#include "core/objects/OSGObject.h"
#include "core/objects/material/VRMaterial.h"


using namespace OSG;

// --------------------------
// ---------Main-------------
// --------------------------

VRGuiGeneral::VRGuiGeneral() {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("on_toggle_bg", [&](OSG::VRGuiSignals::Options o){ setBGType(o["type"]); return true; }, true );
    mgr->addCallback("on_change_bg_color", [&](OSG::VRGuiSignals::Options o){ setBGColor(o["color"]); return true; }, true );
    mgr->addCallback("on_change_bg_path", [&](OSG::VRGuiSignals::Options o){ setBGPath(o["path"]); return true; }, true );
    mgr->addCallback("on_change_bg_ext", [&](OSG::VRGuiSignals::Options o){ setBGExt(o["ext"]); return true; }, true );
    mgr->addCallback("on_enable_splash", [&](OSG::VRGuiSignals::Options o){ enableSplash(toBool(o["state"])); return true; }, true );
    mgr->addCallback("on_change_splash_path", [&](OSG::VRGuiSignals::Options o){ setSplashPath(o["path"]); return true; }, true );

    /*
    setCheckButtonCallback("checkbutton_01", bind(&VRGuiGeneral::toggleFrustumCulling, this) );
    setCheckButtonCallback("checkbutton_02", bind(&VRGuiGeneral::toggleOcclusionCulling, this) );
    setCheckButtonCallback("checkbutton_3", bind(&VRGuiGeneral::toggleDeferredShader, this) );
    setCheckButtonCallback("checkbutton_4", bind(&VRGuiGeneral::toggleSSAO, this) );
    setCheckButtonCallback("checkbutton_5", bind(&VRGuiGeneral::toggleCalib, this) );
    setCheckButtonCallback("checkbutton_6", bind(&VRGuiGeneral::toggleHMDD, this) );
    setCheckButtonCallback("checkbutton_7", bind(&VRGuiGeneral::toggleMarker, this) );
    setCheckButtonCallback("checkbutton_8", bind(&VRGuiGeneral::toggleFXAA, this) );
    setSliderCallback("hscale1", bind(&VRGuiGeneral::setSSAOradius, this, placeholders::_1, placeholders::_2) );
    setSliderCallback("hscale2", bind(&VRGuiGeneral::setSSAOkernel, this, placeholders::_1, placeholders::_2) );
    setSliderCallback("hscale3", bind(&VRGuiGeneral::setSSAOnoise, this, placeholders::_1, placeholders::_2) );
    setButtonCallback("button22", bind(&VRGuiGeneral::dumpOSG, this));
    setRadioButtonCallback("radiobutton13", bind(&VRGuiGeneral::toggleDRendChannel, this));
    setRadioButtonCallback("radiobutton14", bind(&VRGuiGeneral::toggleDRendChannel, this));
    setRadioButtonCallback("radiobutton15", bind(&VRGuiGeneral::toggleDRendChannel, this));
    setRadioButtonCallback("radiobutton16", bind(&VRGuiGeneral::toggleDRendChannel, this));

    fillStringListstore("tfps", { "60", "75", "90", "120", "144" });
    setCombobox("tfpsCombobox", 0);
    setComboboxCallback("tfpsCombobox", bind(&VRGuiGeneral::on_tfps_changed, this) );*/
}

bool VRGuiGeneral::setBGColor(string c) {
    if (updating) return true;
    Color4f col;
    toValue(c, col);
    auto scene = VRScene::getCurrent();
    if (scene) scene->setBackgroundColor(toColor3f(col));
    return true;
}

void VRGuiGeneral::enableSplash(bool b) {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setShowSplash( b );
}

void VRGuiGeneral::setSplashPath(string p) {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setSplashPath( p );
}

void VRGuiGeneral::setBGPath(string p) {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setBackgroundPath( p );
}

void VRGuiGeneral::setBGExt(string e) {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setSkyBGExtension( e );
}

void VRGuiGeneral::setBGType(string t) {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (!scene) return;
    if (t == "solid") scene->setBackground( VRBackground::SOLID );
    if (t == "image") scene->setBackground( VRBackground::IMAGE );
    if (t == "skybox") scene->setBackground( VRBackground::SKYBOX );
    if (t == "sky") scene->setBackground( VRBackground::SKY );
}

void VRGuiGeneral::on_tfps_changed() {
    //float fps = toFloat(getComboboxText("tfpsCombobox"));
    //VRSceneManager::get()->setTargetFPS(fps);
}

bool VRGuiGeneral::setSSAOradius( int st, double d ) {
    if (updating) return false;
    auto scene = VRScene::getCurrent();
    //if (scene) scene->setSSAOradius( getSliderValue("hscale1") );
    return false;
}

bool VRGuiGeneral::setSSAOkernel( int st, double d ) {
    if (updating) return false;
    auto scene = VRScene::getCurrent();
    //if (scene) scene->setSSAOkernel( getSliderValue("hscale2") );
    return false;
}

bool VRGuiGeneral::setSSAOnoise( int st, double d ) {
    if (updating) return false;
    auto scene = VRScene::getCurrent();
    //if (scene) scene->setSSAOnoise( getSliderValue("hscale3") );
    return false;
}

void VRGuiGeneral::dumpOSG() {
    auto scene = VRScene::getCurrent();
    if (!scene) return;

    string pg = scene->getName() + "_osg_dump.osg";
    string pb = scene->getName() + "_osg_dump.osb";
    cout << "dump OSG " << pg << endl;

    SceneFileHandler::the()->write( scene->getRoot()->getNode()->node, pg.c_str() );
    SceneFileHandler::the()->write( scene->getRoot()->getNode()->node, pb.c_str() );

    auto setup = VRSetup::getCurrent();
    for (auto v : setup->getViews()) {
        auto rL = v->getRenderingL();
        auto rR = v->getRenderingR();
        if (rL) { cout << "\nrLEFT " << endl; VRObject::printOSGTree( rL->getRoot()->getNode() ); }
        if (rR) { cout << "\nrRIGHT " << endl; VRObject::printOSGTree( rR->getRoot()->getNode() ); }
    }
}

void VRGuiGeneral::toggleDeferredShader() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    //if (scene) scene->setDeferredShading( getCheckButtonState("checkbutton_3") );
}

void VRGuiGeneral::toggleDRendChannel() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (!scene) return;
    int channel = 0;
    /*if ( getRadioButtonState("radiobutton14") ) channel = 1;
    if ( getRadioButtonState("radiobutton15") ) channel = 2;
    if ( getRadioButtonState("radiobutton16") ) channel = 3;
    if ( getRadioButtonState("radiobutton17") ) channel = 4;*/
    scene->setDeferredChannel(channel);
}

void VRGuiGeneral::toggleSSAO() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    //if (scene) scene->setSSAO( getCheckButtonState("checkbutton_4") );
}

void VRGuiGeneral::toggleHMDD() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    //if (scene) scene->setHMDD( getCheckButtonState("checkbutton_6") );
}

void VRGuiGeneral::toggleFXAA() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    //if (scene) scene->setFXAA( getCheckButtonState("checkbutton_8") );
}

void VRGuiGeneral::toggleCalib() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    //if (scene) scene->setCalib( getCheckButtonState("checkbutton_5") );
}

void VRGuiGeneral::toggleMarker() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    //if (scene) scene->setMarker( getCheckButtonState("checkbutton_7") );
}

void VRGuiGeneral::toggleFrustumCulling() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    //if (scene) scene->setFrustumCulling( getCheckButtonState("checkbutton_01") );
}

void VRGuiGeneral::toggleOcclusionCulling() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    //if (scene) scene->setOcclusionCulling( getCheckButtonState("checkbutton_02") );
}

bool VRGuiGeneral::updateScene() {
	cout << "VRGuiGeneral::updateScene" << endl;
    auto scene = VRScene::getCurrent();
    if (!scene) return true;

    updating = true;

    // background
    Color3f color = scene->getBackgroundColor();
    VRBackground::TYPE t = scene->getBackgroundType();

    uiSignal("set_bg_solid_color", {{"color",toString(color)}});
    uiSignal("set_bg_path", {{"path",scene->getBackgroundPath()}});
    uiSignal("set_bg_file_ext", {{"ext",scene->getSkyBGExtension()}});
    uiSignal("set_enable_splash", {{"show",toString(scene->getShowSplash())}});
    uiSignal("set_splash_path", {{"path",scene->getSplashPath()}});

    if (t == VRBackground::SOLID) uiSignal("set_bg_type", {{"type","solid"}});
    if (t == VRBackground::IMAGE) uiSignal("set_bg_type", {{"type","image"}});
    if (t == VRBackground::SKYBOX) uiSignal("set_bg_type", {{"type","skybox"}});
    if (t == VRBackground::SKY) uiSignal("set_bg_type", {{"type","sky"}});

    // rendering TODO
    /*setToggleButton("checkbutton_01", scene->getFrustumCulling() );
    setToggleButton("checkbutton_02", scene->getOcclusionCulling() );
    setToggleButton("checkbutton_3", scene->getDefferedShading() );
    setToggleButton("checkbutton_4", scene->getSSAO() );
    setToggleButton("checkbutton_6", scene->getHMDD() );
    setToggleButton("checkbutton_8", scene->getFXAA() );

    setSliderValue("hscale1", scene->getSSAOradius());
    setSliderValue("hscale2", scene->getSSAOkernel());
    setSliderValue("hscale3", scene->getSSAOnoise());*/

    updating = false;
    return true;
}


