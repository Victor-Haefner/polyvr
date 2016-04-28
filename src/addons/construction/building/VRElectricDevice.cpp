#include "VRElectricDevice.h"
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


VRElectricDevice::VRElectricDevice(VRObjectPtr obj, VRFunction<bool>* cb) : active(false), callback(cb), EDev(obj) {}

bool VRElectricDevice::isActive() { return active; }

void VRElectricDevice::turnOn() {
    if (!this->active && callback != 0) (*callback)(false);
    this->active = true;
}

void VRElectricDevice::turnOff() {
    if (this->active && callback != 0) (*callback)(false);
    this->active = false;
}

void VRElectricDevice::toggle(VRDevicePtr dev) {
    if (dev != 0 && EDev != 0) {
        VRIntersection ins = dev->intersect(EDev);
        if (!ins.hit) return;
        auto obj = ins.object.lock();
        if ( obj == 0 ) return;
        if ( EDev->find(obj) == 0 ) return;
    }

    if (active) turnOff();
    else turnOn();
}

OSG_END_NAMESPACE;
