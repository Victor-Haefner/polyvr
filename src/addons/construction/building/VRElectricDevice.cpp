#include "VRElectricDevice.h"
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


VRElectricDevice::VRElectricDevice(VRObject* obj, VRFunction<bool>* cb) : active(false), callback(cb), EDev(obj) {}

bool VRElectricDevice::isActive() { return active; }

void VRElectricDevice::turnOn() {
    if (!this->active and callback != 0) (*callback)(false);
    this->active = true;
}

void VRElectricDevice::turnOff() {
    if (this->active and callback != 0) (*callback)(false);
    this->active = false;
}

void VRElectricDevice::toggle(VRDevice* dev) {
    if (dev != 0 and EDev != 0) {
        VRObject* obj = dev->getHitObject();
        if ( obj == 0 ) return;
        if ( EDev->find(obj) == 0 ) return;
    }

    if (active) turnOff();
    else turnOn();
}

OSG_END_NAMESPACE;
