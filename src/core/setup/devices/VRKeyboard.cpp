#include "VRKeyboard.h"

//#include <GL/glut.h>
//#include <OpenSG/OSGBaseInitFunctions.h>
#ifndef WITHOUT_GTK
#include <gdk/gdk.h>
#endif

using namespace OSG;

void VRKeyboard::keyboard(unsigned int k, bool pressed, int x, int y) { change_button(k,pressed); }
void VRKeyboard::keyboard_special(int k, bool pressed, int x, int y) { change_button(k+100,pressed); }

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

bool VRKeyboard::shiftDown() { return (b_state(65505) == 1 || b_state(65506) == 1); } // Shift left and right
bool VRKeyboard::ctrlDown()  { return (b_state(65507) == 1 || b_state(65508) == 1); } // Ctrl  left and right

#ifndef WITHOUT_GTK
void VRKeyboard::setGtkEvent(_GdkEventKey* event) { gdk_event = event; }
_GdkEventKey* VRKeyboard::getGtkEvent() { return gdk_event; }
#endif

