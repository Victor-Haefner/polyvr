#include "VRHaptic.h"
#include "virtuose.h"
#include "core/objects/VRTransform.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRSceneManager.h"
#include <boost/bind.hpp>
#include "core/utils/VRProfiler.h"
#include <time.h>


#define FPS_WATCHDOG_TOLERANCE_CLOCKDELTA 5000
#define FPS_WATCHDOG_TOLERANCE_FPSCHANGE 1000

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
    VRSceneManager::get()->dropUpdateFkt(timestepWatchdog);
    VRSceneManager::get()->dropUpdateFkt(updateFktPre);
    VRSceneManager::get()->dropUpdateFkt(updateFktPost);
    v->disconnect();
}

void VRHaptic::on_scene_changed(VRDevice* dev) {
    v->setBase(0);
    v->detachTransform();

    timestepWatchdog = new VRFunction<int>( "Haptic Timestep Watchdog", boost::bind(&VRHaptic::updateHapticTimestep, this, getBeacon()) );
    updateFktPre = new VRFunction<int>( "Haptic pre update", boost::bind(&VRHaptic::updateHapticPre, this, getBeacon()) );
    updateFktPost = new VRFunction<int>( "Haptic post update", boost::bind(&VRHaptic::updateHapticPost, this, getBeacon()) );
    VRSceneManager::getCurrent()->dropUpdateFkt(timestepWatchdog);
    VRSceneManager::getCurrent()->dropUpdateFkt(updateFktPre);
    VRSceneManager::getCurrent()->dropUpdateFkt(updateFktPost);
    VRSceneManager::getCurrent()->addPhysicsUpdateFunction(timestepWatchdog,false);
    VRSceneManager::getCurrent()->addPhysicsUpdateFunction(updateFktPre,false);
    VRSceneManager::getCurrent()->addPhysicsUpdateFunction(updateFktPost,true);


}

void VRHaptic::applyTransformation(VRTransform* t) { // TODO: rotation
    if (!v->connected()) return;
    t->setMatrix(v->getPose());
}

void VRHaptic::updateHapticTimestep(VRTransform* t) {
    list<VRProfiler::Frame> frames = VRProfiler::get()->getFrames();
    VRProfiler::get()->setActive(true);

    if(frames.size() > 4) {
        //oldest timestep delta
        VRProfiler::Frame tmpOlder = frames.back();
        frames.pop_back();
        VRProfiler::Frame tmpNewer = frames.back();
        int dOld = tmpNewer.t0 - tmpOlder.t0;

        //newest timestep delta
        tmpNewer = frames.front();
        frames.pop_front();
        tmpOlder = frames.front();
        int dNew = tmpNewer.t0 - tmpOlder.t0;
        int diff = dOld - dNew;
        //if it keeps going down/up
        if(abs(diff) > FPS_WATCHDOG_TOLERANCE_CLOCKDELTA) {
            //store in fps_change
            diff < 0 ? fps_change-- : fps_change++;
            cout << fps_change << "\n";

        }
        //if it stopped and the overall drop/increase is big enough
        else {
            if(abs(fps_change) > FPS_WATCHDOG_TOLERANCE_FPSCHANGE) {
                    cout << VRGlobals::get()->PHYSICS_FRAME_RATE << "\n";
            }
            fps_change = 0;

        }

    }
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
void VRHaptic::setBase(VRTransform* trans) {v->setBase(trans);}
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
