#include "VRFlystick.h"
#include "VRSignal.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRFlystick::VRFlystick() : VRDevice("flystick") {
    enableAvatar("cone");
    enableAvatar("ray");
    clearSignals();
}

void VRFlystick::clearSignals() {
    VRDevice::clearSignals();

    addSignal( 0, 0)->add( getDrop() );
    addSignal( 0, 1)->add( addDrag( getBeacon(), 0) );
}

void VRFlystick::update(int Nb, int* buttons, int Ns, float* sliders) {
    for(int i=0;i<Ns;i++) change_slider(i+10, sliders[i]); // slider key has an offset of 10

    for(int key=0; key<Nb; key++) {
        if (BStates.count(key) == 0) continue;
        if (BStates[key] != buttons[key]) change_button(key, buttons[key]);
    }
}

OSG_END_NAMESPACE;
