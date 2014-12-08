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

    updateFkt = new VRFunction<int>( "Haptic update", boost::bind(&VRHaptic::applyTransformation, this, getBeacon()) );
    VRSceneManager::get()->addUpdateFkt(updateFkt);

    store("h_type", &type);
    store("h_ip", &IP);
}

VRHaptic::~VRHaptic() {
    VRSceneManager::get()->dropUpdateFkt(updateFkt);
    v->disconnect();
}

void VRHaptic::applyTransformation(VRTransform* t) { // TODO: rotation
    if (!v->connected()) return;
    t->setMatrix(v->getPose());
    v->updateVirtMech();
}

void VRHaptic::setForce(Vec3f force, Vec3f torque) { v->applyForce(force, torque); }
void VRHaptic::setSimulationScales(float scale, float forces) { v->setSimulationScales(scale, forces); }
Vec3f VRHaptic::getForce(){ v->getForce(); }
void VRHaptic::attachTransform(VRTransform* trans) {v->attachTransform(trans);}
void VRHaptic::detachTransform() {v->detachTransform();}
void VRHaptic::updateVirtMech() {v->updateVirtMech();}


void VRHaptic::setIP(string IP) { this->IP = IP; v->connect(IP); }
string VRHaptic::getIP() { return IP; }

void VRHaptic::setType(string type) { this->type = type; } // TODO: use type for configuration
string VRHaptic::getType() { return type; }

OSG_END_NAMESPACE;
