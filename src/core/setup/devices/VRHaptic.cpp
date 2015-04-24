#include "VRHaptic.h"
#include "virtuose.h"
#include "core/objects/VRTransform.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRSceneManager.h"
#include <boost/bind.hpp>



OSG_BEGIN_NAMESPACE;
using namespace std;

VRHaptic::VRHaptic() : VRDevice("haptic") {
    v = new virtuose();
    setIP("172.22.151.200");

    auto updateObjFkt = new VRFunction<int>( "Haptic object update", boost::bind(&VRHaptic::applyTransformation, this, getBeacon()) );
    VRSceneManager::get()->dropUpdateFkt(updateObjFkt);
    VRSceneManager::get()->addUpdateFkt(updateObjFkt);

    auto fkt = new VRFunction<VRDevice*>( "Haptic on scene changed", boost::bind(&VRHaptic::on_scene_changed, this, _1) );
    VRSceneManager::get()->getSignal_on_scene_load()->add(fkt);

    store("h_type", &type);
    store("h_ip", &IP);
}

VRHaptic::~VRHaptic() {
    VRSceneManager::get()->dropUpdateFkt(updateFktPre);
    VRSceneManager::get()->dropUpdateFkt(updateFktPost);
    v->disconnect();
}

void VRHaptic::on_scene_changed(VRDevice* dev) {
    updateFktPre = new VRFunction<int>( "Haptic pre update", boost::bind(&VRHaptic::updateHapticPre, this, getBeacon()) );
    updateFktPost = new VRFunction<int>( "Haptic post update", boost::bind(&VRHaptic::updateHapticPost, this, getBeacon()) );
    VRSceneManager::getCurrent()->dropUpdateFkt(updateFktPre);
    VRSceneManager::getCurrent()->dropUpdateFkt(updateFktPost);
    VRSceneManager::getCurrent()->addPhysicsUpdateFunction(updateFktPre,false);
    VRSceneManager::getCurrent()->addPhysicsUpdateFunction(updateFktPost,true);

}

void VRHaptic::applyTransformation(VRTransform* t) { // TODO: rotation
    if (!v->connected()) return;
    t->setMatrix(v->getPose());
}

void VRHaptic::updateHapticPre(VRTransform* t) { // TODO: rotation
    //need a recalibration (timestep reset) ?
    //float tempTimeStep = (1.0f/2.0f);
    //cout << "updateHapticPre: timestep: " << tempTimeStep << "\n";
    //if(timestep < tempTimeStep)
    //COMMAND_MODE_VIRTMECH
    updateVirtMechPre();
}

void VRHaptic::updateHapticPost(VRTransform* t) { // TODO: rotation
    //COMMAND_MODE_VIRTMECH
    updateVirtMechPost();
}

void VRHaptic::setForce(Vec3f force, Vec3f torque) { v->applyForce(force, torque); }
Vec3f VRHaptic::getForce() {return v->getForce(); }
void VRHaptic::setSimulationScales(float scale, float forces) { v->setSimulationScales(scale, forces); }
void VRHaptic::attachTransform(VRTransform* trans) {v->attachTransform(trans);}
void VRHaptic::detachTransform() {v->detachTransform();}
void VRHaptic::updateVirtMechPre() {
    v->updateVirtMechPre();
}
void VRHaptic::updateVirtMechPost() {
    v->updateVirtMechPost();
    OSG::Vec3i states = v->getButtonStates();

    //cout << "updateVirtMech b states " << states << endl;

    for (int i=0; i<3; i++) {
        if (states[i] != button_states[i]) {
            //cout << "updateVirtMech trigger " << i << " " << states[i] << endl;
            change_button(i, states[i]);
            button_states[i] = states[i];
        }
    }
}
Vec3i VRHaptic::getButtonStates() {return (v->getButtonStates());}



void VRHaptic::setIP(string IP) { this->IP = IP; v->connect(IP); }
string VRHaptic::getIP() { return IP; }

void VRHaptic::setType(string type) { this->type = type; } // TODO: use type for configuration
string VRHaptic::getType() { return type; }

OSG_END_NAMESPACE;
