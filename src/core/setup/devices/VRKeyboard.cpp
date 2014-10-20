#include "VRKeyboard.h"

#include <GL/glut.h>
#include <OpenSG/OSGBaseInitFunctions.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

void osg_Exit() { osgExit(); exit(0); }
void VRKeyboard::keyboard(unsigned int k, bool pressed, int x, int y) { change_button(k,pressed); }
void VRKeyboard::keyboard_special(int k, bool pressed, int x, int y) { change_button(k+100,pressed); }

VRKeyboard::VRKeyboard() : VRDevice("keyboard") {;}
VRKeyboard::~VRKeyboard() {;}

OSG_END_NAMESPACE;
