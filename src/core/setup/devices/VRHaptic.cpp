#include "VRHaptic.h"
#include "virtuose.h"
#include "core/objects/VRTransform.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRGlobals.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/VRProfiler.h"
#include <time.h>
#include <OpenSG/OSGQuaternion.h>


#define FPS_WATCHDOG_TOLERANCE_EPSILON 0.3
#define FPS_WATCHDOG_COOLDOWNFRAMES 1000

using namespace OSG;


VRHaptic::VRHaptic() : VRDevice("haptic") {
    addBeacon();

    v = new virtuose();

    updatePtr = VRUpdateCb::create( "Haptic object update", bind(&VRHaptic::applyTransformation, this, editBeacon()) );
    VRSceneManager::get()->addUpdateFkt(updatePtr);

    //timestepWatchdog = VRUpdateCb::create( "Haptic Timestep Watchdog", bind(&VRHaptic::updateHapticTimestep, this, editBeacon()) );
    //VRSceneManager::get()->addUpdateFkt(timestepWatchdog);

    onSceneChange = VRDeviceCb::create( "Haptic on scene changed", bind(&VRHaptic::on_scene_changed, this, placeholders::_1) );
    VRSceneManager::get()->getSignal_on_scene_load()->add(onSceneChange);

    store("h_type", &type);
    store("h_ip", &IP);
}

VRHaptic::~VRHaptic() {
    VRScene::getCurrent()->dropPhysicsUpdateFunction(updateFktPre,false);
    VRScene::getCurrent()->dropPhysicsUpdateFunction(updateFktPost,true);
    v->disconnect();
}

VRHapticPtr VRHaptic::create() {
    auto d = VRHapticPtr( new VRHaptic() );
    d->initIntersect(d);
    return d;
}

VRHapticPtr VRHaptic::ptr() { return static_pointer_cast<VRHaptic>( shared_from_this() ); }

void VRHaptic::on_scene_changed(VRDeviceWeakPtr dev) {
    VRScene::getCurrent()->dropPhysicsUpdateFunction(updateFktPre,false);
    VRScene::getCurrent()->dropPhysicsUpdateFunction(updateFktPost,true);
    //disconnect
    v->setBase(0);
    v->detachTransform();
    v->disconnect();

    updateFktPre = VRUpdateCb::create( "Haptic pre update", bind(&VRHaptic::updateHapticPre, this, editBeacon()) );
    updateFktPost = VRUpdateCb::create( "Haptic post update", bind(&VRHaptic::updateHapticPost, this, editBeacon()) );
    VRScene::getCurrent()->addPhysicsUpdateFunction(updateFktPre,false);
    VRScene::getCurrent()->addPhysicsUpdateFunction(updateFktPost,true);

    setIP(IP); //reconnect
}

void VRHaptic::applyTransformation(VRTransformPtr t) {
    if (!v->connected()) return;
    t->setMatrix(v->getPose());
}

void VRHaptic::updateHapticTimestep(VRTransformPtr t) {
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
                on_scene_changed(ptr());
            }
        }
}

void VRHaptic::updateHapticPre(VRTransformPtr t) { // TODO: rotation
     if (!v->connected()) return;
   //COMMAND_MODE_VIRTMECH
    updateVirtMechPre();
}

void VRHaptic::updateHapticPost(VRTransformPtr t) { // TODO: rotation
    //cout << " VRHaptic::updateHapticPost " << v->connected() << endl;
     if (!v->connected()) return;
   //COMMAND_MODE_VIRTMECH
    updateVirtMechPost();
}

void VRHaptic::updateVirtMechPre() {
    v->updateVirtMechPre();
}

void VRHaptic::updateVirtMechPost() {
    v->updateVirtMechPost();
    OSG::Vec3i states = v->getButtonStates();

    for (int i=0; i<3; i++) {
        if (states[i] != button_states[i]) {
            // leads to unexpected behaviour: (virtual obj is set to origin immediately)
            // hangs ..
            //change_button(i, states[i]);
            //button_states[i] = states[i];
        }
    }
}

void VRHaptic::setForce(Vec3d force, Vec3d torque) { v->applyForce(force, torque); }
Vec3d VRHaptic::getForce() {return v->getForce(); }
void VRHaptic::setSimulationScales(float scale, float forces) { v->setSimulationScales(scale, forces); }
void VRHaptic::attachTransform(VRTransformPtr trans) {v->attachTransform(trans);}
void VRHaptic::setBase(VRTransformPtr trans) {v->setBase(trans);}
void VRHaptic::detachTransform() {v->detachTransform();}
Vec3i VRHaptic::getButtonStates() { return (v->getButtonStates()); }

void VRHaptic::setIP(string IP) { this->IP = IP; v->connect(IP,1.0f/(float)VRGlobals::PHYSICS_FRAME_RATE.fps);}
void VRHaptic::setType(string type) { this->type = type; } // TODO: use type for configuration
string VRHaptic::getIP() { return IP; }
string VRHaptic::getType() { return type; }

string VRHaptic::getDeamonState() { return v->getDeamonState(); }
string VRHaptic::getDeviceState() { return v->getDeviceState(); }

vector<string> VRHaptic::getDevTypes() {
    return {"Virtuose 6D Desktop", "Virtuose 6D35-45", "Virtuose 6D40-40", "INCA 6D"};
}

void VRHaptic::runTest1() {
    cout << " --- run haptic test 1 --- " << endl;
    auto vc = virtOpen("172.22.151.200");
    virtClose(vc);
    /*v = new virtuose();
    v->disconnect();
    setIP("172.22.151.200");

    updatePtr = VRUpdateCb::create( "Haptic object update", bind(&VRHaptic::applyTransformation, this, editBeacon()) );
    VRSceneManager::get()->addUpdateFkt(updatePtr);

    on_scene_changed(0);*/
}




