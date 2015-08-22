
#include "VRGuiGeneral.h"
#include "VRGuiUtils.h"
#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/textview.h>
#include <gtkmm/combobox.h>
#include <gtkmm/cellrenderercombo.h>

#include<OpenSG/OSGSceneFileHandler.h>
#include "core/scene/VRScene.h"
#include "core/objects/material/VRMaterial.h"


OSG_BEGIN_NAMESPACE;
using namespace std;

// --------------------------
// ---------Main-------------
// --------------------------

VRGuiGeneral::VRGuiGeneral() {
    setCheckButtonCallback("radiobutton5", sigc::mem_fun(*this, &VRGuiGeneral::setMode) );
    setCheckButtonCallback("radiobutton4", sigc::mem_fun(*this, &VRGuiGeneral::setMode) );
    setCheckButtonCallback("checkbutton_01", sigc::mem_fun(*this, &VRGuiGeneral::toggleFrustumCulling) );
    setCheckButtonCallback("checkbutton_02", sigc::mem_fun(*this, &VRGuiGeneral::toggleOcclusionCulling) );
    setCheckButtonCallback("checkbutton_2", sigc::mem_fun(*this, &VRGuiGeneral::toggleTwoSided) );
    setCheckButtonCallback("checkbutton_3", sigc::mem_fun(*this, &VRGuiGeneral::toggleDefferedShader) );
    setCheckButtonCallback("checkbutton_4", sigc::mem_fun(*this, &VRGuiGeneral::toggleSSAO) );
    setButtonCallback("button22", sigc::mem_fun(*this, &VRGuiGeneral::dumpOSG) );
    setColorChooser("bg_solid", sigc::mem_fun(*this, &VRGuiGeneral::setColor) );
    setEntryCallback("entry42", sigc::mem_fun(*this, &VRGuiGeneral::setPath));
    setEntryCallback("entry14", sigc::mem_fun(*this, &VRGuiGeneral::setExtension));
}

void VRGuiGeneral::dumpOSG() {
     VRScene* scene = VRSceneManager::getCurrent();
     if (scene == 0) return;

     string pg = scene->getName() + "_osg_dump.osg";
     string pb = scene->getName() + "_osg_dump.osb";

     SceneFileHandler::the()->write( scene->getRoot()->getNode(), pg.c_str() );
     SceneFileHandler::the()->write( scene->getRoot()->getNode(), pb.c_str() );
}

bool VRGuiGeneral::setColor(GdkEventButton* b) {
    if (updating) return true;

    Color3f col = VRSceneManager::getCurrent()->getBackgroundColor();
    Color4f c = chooseColor("bg_solid", toColor4f(col));
    VRSceneManager::getCurrent()->setBackgroundColor(toColor3f(c));
    return true;
}

void VRGuiGeneral::setPath() {
    if (updating) return;
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    scene->setBackgroundPath( getTextEntry("entry42") );
}

void VRGuiGeneral::setExtension() {
    if (updating) return;
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    scene->setSkyBGExtension( getTextEntry("entry14") );
}

void VRGuiGeneral::setMode() {
    if (updating) return;

    VRBackground::TYPE t = VRBackground::SOLID;
    if ( getCheckButtonState("radiobutton4") ) t = VRBackground::IMAGE;
    if ( getCheckButtonState("radiobutton5") ) t = VRBackground::SKY;
    VRSceneManager::getCurrent()->setBackground( t );

    setEntrySensitivity("entry14", t == VRBackground::SKY);
    setEntrySensitivity("entry42", t == VRBackground::SKY || t == VRBackground::IMAGE);
}

void VRGuiGeneral::toggleDefferedShader() {
    if (updating) return;
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    scene->setDefferedShading( getCheckButtonState("checkbutton_3") );
}

void VRGuiGeneral::toggleSSAO() {
    if (updating) return;
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    scene->setSSAO( getCheckButtonState("checkbutton_4") );
}

void VRGuiGeneral::toggleFrustumCulling() {
    if (updating) return;

    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;

    scene->setFrustumCulling( getCheckButtonState("checkbutton_01") );
}

void VRGuiGeneral::toggleOcclusionCulling() {
    if (updating) return;

    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;

    scene->setOcclusionCulling( getCheckButtonState("checkbutton_02") );
}

void VRGuiGeneral::toggleTwoSided() {
    if (updating) return;

    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;

    scene->setTwoSided( getCheckButtonState("checkbutton_2") );
}

void VRGuiGeneral::updateScene() {
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;

    updating = true;

    // background
    Color3f col = scene->getBackgroundColor();
    VRBackground::TYPE t = scene->getBackgroundType();

    setColorChooserColor("bg_solid", Color3f(col[0], col[1], col[2]));
    setTextEntry("entry42", scene->getBackgroundPath());
    setEntrySensitivity("entry14", t == VRBackground::SKY);
    if (t == VRBackground::SKY) setTextEntry("entry14", scene->getSkyBGExtension());

    setCheckButton("radiobutton5", t == VRBackground::SKY);
    setCheckButton("radiobutton4", t == VRBackground::IMAGE);

    // rendering
    setCheckButton("checkbutton_01", scene->getFrustumCulling() );
    setCheckButton("checkbutton_02", scene->getOcclusionCulling() );
    setCheckButton("checkbutton_2", scene->getTwoSided() );
    setCheckButton("checkbutton_3", scene->getDefferedShading() );
    setCheckButton("checkbutton_4", scene->getSSAO() );

    updating = false;
}


OSG_END_NAMESPACE;
