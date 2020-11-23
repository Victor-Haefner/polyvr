#include <gtk/gtk.h>
#include "VRGuiGeneral.h"
#include "VRGuiUtils.h"
#include "VRGuiFile.h"

#include <OpenSG/OSGSceneFileHandler.h>
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
    setCheckButtonCallback("radiobutton5", bind(&VRGuiGeneral::setMode, this) );
    setCheckButtonCallback("radiobutton18", bind(&VRGuiGeneral::setMode, this) );
    setCheckButtonCallback("radiobutton4", bind(&VRGuiGeneral::setMode, this) );
    setCheckButtonCallback("checkbutton_01", bind(&VRGuiGeneral::toggleFrustumCulling, this) );
    setCheckButtonCallback("checkbutton_02", bind(&VRGuiGeneral::toggleOcclusionCulling, this) );
    setCheckButtonCallback("checkbutton_2", bind(&VRGuiGeneral::toggleTwoSided, this) );
    setCheckButtonCallback("checkbutton_3", bind(&VRGuiGeneral::toggleDeferredShader, this) );
    setCheckButtonCallback("checkbutton_4", bind(&VRGuiGeneral::toggleSSAO, this) );
    setCheckButtonCallback("checkbutton_5", bind(&VRGuiGeneral::toggleCalib, this) );
    setCheckButtonCallback("checkbutton_6", bind(&VRGuiGeneral::toggleHMDD, this) );
    setCheckButtonCallback("checkbutton_7", bind(&VRGuiGeneral::toggleMarker, this) );
    setCheckButtonCallback("checkbutton_8", bind(&VRGuiGeneral::toggleFXAA, this) );
    setSliderCallback("hscale1", bind(&VRGuiGeneral::setSSAOradius, this, placeholders::_1, placeholders::_2) );
    setSliderCallback("hscale2", bind(&VRGuiGeneral::setSSAOkernel, this, placeholders::_1, placeholders::_2) );
    setSliderCallback("hscale3", bind(&VRGuiGeneral::setSSAOnoise, this, placeholders::_1, placeholders::_2) );
    setColorChooser("bg_solid", bind(&VRGuiGeneral::setColor, this, placeholders::_1) );
    setEntryCallback("entry42", bind(&VRGuiGeneral::setPath, this));
    setEntryCallback("entry14", bind(&VRGuiGeneral::setExtension, this));
    setButtonCallback("button18", bind(&VRGuiGeneral::openBGpath, this));
    setButtonCallback("button22", bind(&VRGuiGeneral::dumpOSG, this));
    setRadioButtonCallback("radiobutton13", bind(&VRGuiGeneral::toggleDRendChannel, this));
    setRadioButtonCallback("radiobutton14", bind(&VRGuiGeneral::toggleDRendChannel, this));
    setRadioButtonCallback("radiobutton15", bind(&VRGuiGeneral::toggleDRendChannel, this));
    setRadioButtonCallback("radiobutton16", bind(&VRGuiGeneral::toggleDRendChannel, this));

    fillStringListstore("tfps", { "60", "75", "90", "120", "144" });
    setCombobox("tfpsCombobox", 0);
    setComboboxCallback("tfpsCombobox", bind(&VRGuiGeneral::on_tfps_changed, this) );
}

void VRGuiGeneral::on_bg_path_choose() {
    string path = VRGuiFile::getPath();
    setTextEntry("entry42", path);
    setPath();
}

void VRGuiGeneral::openBGpath() {
    VRGuiFile::gotoPath("./");
    VRGuiFile::setCallbacks(bind(&VRGuiGeneral::on_bg_path_choose, this));
    VRGuiFile::open("Open", GTK_FILE_CHOOSER_ACTION_OPEN, "Choose image");
}

void VRGuiGeneral::on_tfps_changed() {
    float fps = toFloat(getComboboxText("tfpsCombobox"));
    VRSceneManager::get()->setTargetFPS(fps);
}

bool VRGuiGeneral::setSSAOradius( int st, double d ) {
    if (updating) return false;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setSSAOradius( getSliderState("hscale1") );
    return true;
}

bool VRGuiGeneral::setSSAOkernel( int st, double d ) {
    if (updating) return false;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setSSAOkernel( getSliderState("hscale2") );
    return true;
}

bool VRGuiGeneral::setSSAOnoise( int st, double d ) {
    if (updating) return false;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setSSAOnoise( getSliderState("hscale3") );
    return true;
}

void VRGuiGeneral::dumpOSG() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

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

bool VRGuiGeneral::setColor(GdkEventButton* b) {
    if (updating) return true;

    auto scene = VRScene::getCurrent();
    Color3f col = scene->getBackgroundColor();
    Color4f c = chooseColor("bg_solid", toColor4f(col));
    scene->setBackgroundColor(toColor3f(c));
    return true;
}

void VRGuiGeneral::setPath() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->setBackgroundPath( getTextEntry("entry42") );
}

void VRGuiGeneral::setExtension() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->setSkyBGExtension( getTextEntry("entry14") );
}

void VRGuiGeneral::setMode() {
    if (updating) return;

    VRBackground::TYPE t = VRBackground::SOLID;
    if ( getCheckButtonState("radiobutton4") ) t = VRBackground::IMAGE;
    if ( getCheckButtonState("radiobutton5") ) t = VRBackground::SKYBOX;
    if ( getCheckButtonState("radiobutton18") ) t = VRBackground::SKY;
    auto scene = VRScene::getCurrent();
    scene->setBackground( t );

    setWidgetSensitivity("entry14", t == VRBackground::SKYBOX);
    setWidgetSensitivity("entry42", t == VRBackground::SKYBOX || t == VRBackground::IMAGE);
    setWidgetSensitivity("button18", t == VRBackground::IMAGE);
}

void VRGuiGeneral::toggleDeferredShader() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setDeferredShading( getCheckButtonState("checkbutton_3") );
}

void VRGuiGeneral::toggleDRendChannel() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (!scene) return;
    int channel = 0;
    if ( getRadioButtonState("radiobutton14") ) channel = 1;
    if ( getRadioButtonState("radiobutton15") ) channel = 2;
    if ( getRadioButtonState("radiobutton16") ) channel = 3;
    if ( getRadioButtonState("radiobutton17") ) channel = 4;
    scene->setDeferredChannel(channel);
}

void VRGuiGeneral::toggleSSAO() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setSSAO( getCheckButtonState("checkbutton_4") );
}

void VRGuiGeneral::toggleHMDD() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setHMDD( getCheckButtonState("checkbutton_6") );
}

void VRGuiGeneral::toggleFXAA() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setFXAA( getCheckButtonState("checkbutton_8") );
}

void VRGuiGeneral::toggleCalib() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setCalib( getCheckButtonState("checkbutton_5") );
}

void VRGuiGeneral::toggleMarker() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setMarker( getCheckButtonState("checkbutton_7") );
}

void VRGuiGeneral::toggleFrustumCulling() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setFrustumCulling( getCheckButtonState("checkbutton_01") );
}

void VRGuiGeneral::toggleOcclusionCulling() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setOcclusionCulling( getCheckButtonState("checkbutton_02") );
}

void VRGuiGeneral::toggleTwoSided() {
    if (updating) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->setTwoSided( getCheckButtonState("checkbutton_2") );
}

void VRGuiGeneral::updateScene() {
	cout << "VRGuiGeneral::updateScene" << endl;
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    updating = true;

    // background
    Color3f col = scene->getBackgroundColor();
    VRBackground::TYPE t = scene->getBackgroundType();

    setColorChooserColor("bg_solid", Color3f(col[0], col[1], col[2]));
    setTextEntry("entry42", scene->getBackgroundPath());
    setWidgetSensitivity("entry14", t == VRBackground::SKYBOX);
    if (t == VRBackground::SKYBOX) setTextEntry("entry14", scene->getSkyBGExtension());

    setToggleButton("radiobutton18", t == VRBackground::SKY);
    setToggleButton("radiobutton5", t == VRBackground::SKYBOX);
    setToggleButton("radiobutton4", t == VRBackground::IMAGE);

    // rendering
    setToggleButton("checkbutton_01", scene->getFrustumCulling() );
    setToggleButton("checkbutton_02", scene->getOcclusionCulling() );
    setToggleButton("checkbutton_2", scene->getTwoSided() );
    setToggleButton("checkbutton_3", scene->getDefferedShading() );
    setToggleButton("checkbutton_4", scene->getSSAO() );
    setToggleButton("checkbutton_6", scene->getHMDD() );
    setToggleButton("checkbutton_8", scene->getFXAA() );

    updating = false;
}


