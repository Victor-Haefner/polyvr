#include "VRKeyboard.h"

using namespace OSG;

void VRKeyboard::applyEvents() {
    for (auto e : delayedEvents) {
        if (e[0] == 0) keyboard(e[1], e[2], e[3], e[4], false);
        if (e[0] == 1) keyboard_special(e[1], e[2], e[3], e[4], false);
    }
    delayedEvents.clear();
}

void VRKeyboard::keyboard(unsigned int k, bool pressed, int x, int y, bool delayed) {
    event.keyval = k;
    event.state = pressed;

    if (delayed && 0) { // TODO: delay deaktivated because CEF doenst like it at all!
        delayedEvents.push_back( {0,int(k),int(pressed),x,y} );
        return;
    }

    //cout << "VRKeyboard::keyboard " << k << " " << pressed << " " << Vec2i(x,y) << endl;
    change_button(k,pressed);
}

void VRKeyboard::keyboard_special(int k, bool pressed, int x, int y, bool delayed) {
    k += 256;
    event.keyval = k;
    event.state = pressed;

    if (delayed && 0) { // TODO: delay deaktivated because CEF doenst like it at all!
        delayedEvents.push_back( {1,k,pressed,x,y} );
        return;
    }

    //cout << "VRKeyboard::keyboard_special " << k << " " << pressed << " " << Vec2i(x,y) << endl;
    change_button(k,pressed);
}

VRKeyboard::VRKeyboard() : VRDevice("keyboard") {
    addBeacon();
}

VRKeyboard::~VRKeyboard() {;}

VRKeyboardPtr VRKeyboard::create() {
    auto d = VRKeyboardPtr(new VRKeyboard());
    d->initIntersect(d);
    return d;
}

VRKeyboardPtr VRKeyboard::ptr() { return static_pointer_cast<VRKeyboard>( shared_from_this() ); }

bool VRKeyboard::shiftDown() { return (b_state(368) == 1 || b_state(369) == 1); } // Shift left and right
bool VRKeyboard::ctrlDown()  { return (b_state(370) == 1 || b_state(371) == 1); } // Ctrl  left and right
bool VRKeyboard::altDown()  { return (b_state(372) == 1 || b_state(373) == 1); } // Alt
bool VRKeyboard::lockDown()  { return (b_state(144) == 1); } // Num Lock

VRKeyboard::KeyEvent& VRKeyboard::getEvent() { return event; }

