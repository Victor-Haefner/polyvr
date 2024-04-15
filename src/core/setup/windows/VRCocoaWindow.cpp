#include "VRCocoaWindow.h"

using namespace OSG;

VRCocoaWindow::VRCocoaWindow() {}
VRCocoaWindow::~VRCocoaWindow() {}

VRCocoaWindowPtr VRCocoaWindow::create() { return VRCocoaWindowPtr( new VRCocoaWindow() ); }
VRCocoaWindowPtr VRCocoaWindow::ptr() { return static_pointer_cast<VRCocoaWindow>(shared_from_this()); }

void VRCocoaWindow::render(bool fromThread) {}

void VRCocoaWindow::save(XMLElementPtr node) {}
void VRCocoaWindow::load(XMLElementPtr node) {}

void VRCocoaWindow::onDisplay() {}
void VRCocoaWindow::onMouse(int b, int s, int x, int y) {}
void VRCocoaWindow::onMotion(int x, int y) {}
void VRCocoaWindow::onKeyboard(int k, int s, int x, int y) {}
void VRCocoaWindow::onKeyboard_special(int k, int s, int x, int y) {}
