
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
    updating = false;
    setCheckButtonCallback("checkbutton29", sigc::mem_fun(*this, &VRGuiGeneral::setMode) );
    setCheckButtonCallback("checkbutton30", sigc::mem_fun(*this, &VRGuiGeneral::setMode) );
    setCheckButtonCallback("checkbutton_01", sigc::mem_fun(*this, &VRGuiGeneral::toggleFrustumCulling) );
    setCheckButtonCallback("checkbutton_02", sigc::mem_fun(*this, &VRGuiGeneral::toggleOcclusionCulling) );
    setButtonCallback("button22", sigc::mem_fun(*this, &VRGuiGeneral::dumpOSG) );
    setColorChooser("bg_solid", sigc::mem_fun(*this, &VRGuiGeneral::setColor) );
    setEntryCallback("entry42", sigc::mem_fun(*this, &VRGuiGeneral::setPath));
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

    string s = getTextEntry("entry42");
    scene->setBackgroundPath(s);
}

void VRGuiGeneral::setMode() {
    if (updating) return;

    VRBackground::TYPE t = VRBackground::SOLID;
    if ( getCheckButtonState("checkbutton30") ) t = VRBackground::IMAGE;
    if ( getCheckButtonState("checkbutton29") ) t = VRBackground::SKY;
    VRSceneManager::getCurrent()->setBackground( t );
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

void VRGuiGeneral::updateScene() {
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;

    updating = true;

    // background
    Color3f col = scene->getBackgroundColor();
    string p = scene->getBackgroundPath();
    VRBackground::TYPE t = scene->getBackgroundType();
    setColorChooserColor("bg_solid", Color3f(col[0], col[1], col[2]));
    setTextEntry("entry42", p);
    setCheckButton("checkbutton29", t == VRBackground::SKY);
    setCheckButton("checkbutton30", t == VRBackground::IMAGE);

    // rendering
    setCheckButton("checkbutton_01", scene->getFrustumCulling() );
    setCheckButton("checkbutton_02", scene->getOcclusionCulling() );


    updating = false;
}


OSG_END_NAMESPACE;
