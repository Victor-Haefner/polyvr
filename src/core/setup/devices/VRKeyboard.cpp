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
    event.keyval = k;
    event.state = pressed;

    if (delayed && 0) { // TODO: delay deaktivated because CEF doenst like it at all!
        delayedEvents.push_back( {1,k,pressed,x,y} );
        return;
    }

    //cout << "VRKeyboard::keyboard_special " << k << " " << pressed << " " << Vec2i(x,y) << endl;
    change_button(k+1000,pressed);
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

bool VRKeyboard::shiftDown() { return (b_state(1112) == 1 || b_state(1113) == 1); } // Shift left and right
bool VRKeyboard::ctrlDown()  { return (b_state(1114) == 1 || b_state(1115) == 1); } // Ctrl  left and right
bool VRKeyboard::altDown()  { return (b_state(1116) == 1); } // Alt
bool VRKeyboard::lockDown()  { return false; } // TODO

VRKeyboard::KeyEvent& VRKeyboard::getEvent() { return event; }

