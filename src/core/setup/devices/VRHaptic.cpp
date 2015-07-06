#include "VRHaptic.h"
#include "virtuose.h"
#include "core/objects/VRTransform.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRSceneManager.h"
#include <boost/bind.hpp>
#include "core/utils/VRProfiler.h"
#include <time.h>


#define FPS_WATCHDOG_TOLERANCE_EPSILON 0.3
#define FPS_WATCHDOG_COOLDOWNFRAMES 1000

OSG_BEGIN_NAMESPACE;
using namespace std;

VRHaptic::VRHaptic() : VRDevice("haptic") {
    v = new virtuose();
    v->disconnect();
    setIP("172.22.151.200");

    auto updateObjFkt = new VRFunction<int>( "Haptic object update", boost::bind(&VRHaptic::applyTransformation, this, editBeacon()) );
    //VRSceneManager::get()->dropUpdateFkt(updateObjFkt);
    VRSceneManager::get()->addUpdateFkt(updateObjFkt);

    auto fkt = new VRFunction<VRDevice*>( "Haptic on scene changed", boost::bind(&VRHaptic::on_scene_changed, this, _1) );
    VRSceneManager::get()->getSignal_on_scene_load()->add(fkt);

    store("h_type", &type);
    store("h_ip", &IP);
}

VRHaptic::~VRHaptic() {
    VRSceneManager::getCurrent()->dropUpdateFkt(timestepWatchdog);
    VRSceneManager::getCurrent()->dropPhysicsUpdateFunction(updateFktPre,false);
    VRSceneManager::getCurrent()->dropPhysicsUpdateFunction(updateFktPost,true);
    v->disconnect();
}

void VRHaptic::on_scene_changed(VRDevice* dev) {

    VRSceneManager::getCurrent()->dropPhysicsUpdateFunction(updateFktPre,false);
    VRSceneManager::getCurrent()->dropPhysicsUpdateFunction(updateFktPost,true);
    //disconnect
    v->setBase(0);
    v->detachTransform();
    v->disconnect();

    timestepWatchdog = new VRFunction<int>( "Haptic Timestep Watchdog", boost::bind(&VRHaptic::updateHapticTimestep, this, editBeacon()) );
    updateFktPre = new VRFunction<int>( "Haptic pre update", boost::bind(&VRHaptic::updateHapticPre, this, editBeacon()) );
    updateFktPost = new VRFunction<int>( "Haptic post update", boost::bind(&VRHaptic::updateHapticPost, this, editBeacon()) );

    VRSceneManager::getCurrent()->dropUpdateFkt(timestepWatchdog);

    VRSceneManager::getCurrent()->addUpdateFkt(timestepWatchdog);
    VRSceneManager::getCurrent()->addPhysicsUpdateFunction(updateFktPre,false);
    VRSceneManager::getCurrent()->addPhysicsUpdateFunction(updateFktPost,true);


    //reconnect
    setIP("172.22.151.200");

}

void VRHaptic::applyTransformation(VRTransform* t) { // TODO: rotation
    if (!v->connected()) return;
    t->setMatrix(v->getPose());
}

void VRHaptic::updateHapticTimestep(VRTransform* t) {
    list<VRProfiler::Frame> frames = VRProfiler::get()->getFrames();

        VRProfiler::Frame tmpOlder;
        VRProfiler::Frame tmpNewer;
        int listSize = frames.size();
        double av = 1.;
        //average time delta ratio    = O(listsize)
        for(int i = 0 ; i < (listSize - 1);i++) {
            tmpOlder = frames.back();
            frames.pop_back();
            //there are frames left in history
            if(frames.size() > 0) {
                tmpNewer = frames.back();
                //timestamp older frame divided by timestamp newer frame
                av +=  ((double)tmpOlder.t0 / (double)tmpNewer.t0);
            }
            //tmpOlder is the newest frame
            else {
                break;
            }
        }
        //average
        av /= (double)listSize;
        frames.clear();

        //fps keeps changing (absolute ratio is greater than 1+epsilon
        if(abs(av - 1.) > FPS_WATCHDOG_TOLERANCE_EPSILON) {
            fps_change++;
            fps_stable = 0;
        }
        //fps doesn't change
        else {
            //fps marked as recently unstable -> cooldown
            fps_change-= fps_stable == 0 ? 1 : 0;
            //cooldown up to -FPS_WATCHDOG_COOLDOWNFRAMES
            if(fps_change < -FPS_WATCHDOG_COOLDOWNFRAMES) {
                //fps now stable
                fps_change = 0;
                fps_stable = 1;
                //cout << "reconnect haptic" << VRGlobals::get()->PHYSICS_FRAME_RATE << "\n";
                //reconnect haptic
                on_scene_changed(this);
            }
        }
}

void VRHaptic::updateHapticPre(VRTransform* t) { // TODO: rotation
     if (!v->connected()) return;
   //COMMAND_MODE_VIRTMECH
    updateVirtMechPre();
}

void VRHaptic::updateHapticPost(VRTransform* t) { // TODO: rotation
     if (!v->connected()) return;
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

            // leads to unexpected behaviour: (virtual obj is set to origin immediately)
            //change_button(i, states[i]);
            //button_states[i] = states[i];
        }
    }
}
Vec3i VRHaptic::getButtonStates() {return (v->getButtonStates());}



void VRHaptic::setIP(string IP) { this->IP = IP; v->connect(IP,1.0f/(float)VRGlobals::get()->PHYSICS_FRAME_RATE);}
string VRHaptic::getIP() { return IP; }

void VRHaptic::setType(string type) { this->type = type; } // TODO: use type for configuration
string VRHaptic::getType() { return type; }

OSG_END_NAMESPACE;
