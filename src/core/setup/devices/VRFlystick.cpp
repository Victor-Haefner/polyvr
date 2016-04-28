#include "VRFlystick.h"
#include "VRSignal.h"
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRFlystick::VRFlystick() : VRDevice("flystick") {
    enableAvatar("cone");
    enableAvatar("ray");
}

VRFlystickPtr VRFlystick::create() {
    auto d = VRFlystickPtr(new VRFlystick());
    d->initIntersect(d);
    d->clearSignals();
    return d;
}

VRFlystickPtr VRFlystick::ptr() { return static_pointer_cast<VRFlystick>( shared_from_this() ); }

void VRFlystick::clearSignals() {
    VRDevice::clearSignals();

    addSignal( 0, 0)->add( getDrop() );
    addSignal( 0, 1)->add( addDrag( getBeacon() ) );
}

void VRFlystick::update(vector<int> buttons, vector<float> sliders) {
    for(uint i=0; i<sliders.size(); i++) change_slider(i+10, sliders[i]); // art slider key has an offset of 10

    for(uint k=0; k<buttons.size(); k++) {
        if (BStates[k] != buttons[k] || BStates.count(k) == 0) change_button(k, buttons[k]);
    }
}

OSG_END_NAMESPACE;
