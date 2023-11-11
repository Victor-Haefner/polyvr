#include <OpenSG/OSGGLUT.h>
#include <GL/freeglut.h>

#include "VRGlutWindow.h"
#include "core/utils/VROptions.h"

#include "../devices/VRMouse.h"
#include "../devices/VRKeyboard.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRWebXR.h"

#include "glut/VRGlutExtensions.h"

#ifndef WITHOUT_OPENVR
#include "VRHeadMountedDisplay.h"
#endif

OSG_BEGIN_NAMESPACE;
using namespace std;

map<int, VRGlutWindow*> glutWindows;

VRGlutWindow* getCurrentWindow() {
#ifndef WASM
    return glutWindows[glutGetWindow()];
#else
    return glutWindows.begin()->second;
#endif
}

void glutDisplay() { getCurrentWindow()->onDisplay(); }
void glutResize(int w, int h) { getCurrentWindow()->resize(w, h); }
void glutMouse(int b, int s, int x, int y) { getCurrentWindow()->onMouse(b ,s ,x ,y); }
void glutMotion(int x, int y) { getCurrentWindow()->onMotion(x, y); }
void glutKeyboard(unsigned char k, int x, int y) { getCurrentWindow()->onKeyboard(k, 1, x, y); }
void glutSpecial(int k, int x, int y) { getCurrentWindow()->onKeyboard_special(k, 1, x, y); }
void glutKeyboardUp(unsigned char k, int x, int y) { getCurrentWindow()->onKeyboard(k, 0, x, y); }
void glutSpecialUp(int k, int x, int y) { getCurrentWindow()->onKeyboard_special(k, 0, x, y); }

VRGlutWindow::VRGlutWindow() {
    cout << "Glut: New Window" << endl;
    type = "glut";

#ifndef WITHOUT_OPENVR
    if (VRHeadMountedDisplay::checkDeviceAttached())
        hmd = VRHeadMountedDisplay::create();
#endif

    int width = 800;//20;
    int height = 600;//10;

    initGlut();

    cout << " Glut create window" << endl;
    glutInitWindowSize(width, height);
    winID = glutCreateWindow("PolyVR");
    cout << "  window ID: " << winID << endl;

    GLUTWindowMTRecPtr win = GLUTWindow::create();
    _win = win;
    win->setGlutId(winID);
    win->setSize(width, height);
    cout << "  init OpenSG GLUT window" << endl;
    win->init();

    glutWindows[winID] = this;

    glutDisplayFunc(glutDisplay);
    glutReshapeFunc(glutResize);
    glutKeyboardFunc(glutKeyboard);
    glutSpecialFunc(glutSpecial);
    glutKeyboardUpFunc(glutKeyboardUp);
    glutSpecialUpFunc(glutSpecialUp);
    glutMotionFunc(glutMotion);
    glutMouseFunc(glutMouse);

    bool fullscreen = VROptions::get()->getOption<bool>("fullscreen");
    if (fullscreen) glutFullScreen();
    cout << " Glut window initiated" << endl;
}

VRGlutWindow::~VRGlutWindow() {
    glutDestroyWindow(winID);
    win = NULL;
}

VRGlutWindowPtr VRGlutWindow::ptr() { return static_pointer_cast<VRGlutWindow>( shared_from_this() ); }
VRGlutWindowPtr VRGlutWindow::create() { return VRGlutWindowPtr(new VRGlutWindow() ); }

void VRGlutWindow::initGlut() {
    static bool glutInititated = false;
    if (glutInititated) return;
    glutInititated = true;
    cout << " init GLUT (VRGlutWindow)";

    putenv((char*)"__GL_SYNC_TO_VBLANK=1");
    glutInit(&VROptions::get()->argc, VROptions::get()->argv);

#ifdef WASM
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL | GLUT_MULTISAMPLE);
#else
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    if (VROptions::get()->getOption<bool>("active_stereo"))
        glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STEREO | GLUT_STENCIL | GLUT_MULTISAMPLE);
    else glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL | GLUT_MULTISAMPLE);
#endif

    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

    cout << " ..done " << endl;
}

void VRGlutWindow::save(XMLElementPtr node) { VRWindow::save(node); }
void VRGlutWindow::load(XMLElementPtr node) { VRWindow::load(node); }

void VRGlutWindow::onMouse(int b, int s, int x, int y) {
    // swap mouse wheel
    if (b == 3) b = 4;
    else if (b == 4) b = 3;

    cout << "VRGlutWindow::onMouse " << Vec4i(b, s, x, y) << endl;
    if (auto m = getMouse()) m->mouse(b, s, x, y, 0);
}

void VRGlutWindow::onMotion(int x, int y) {
    cout << "VRGlutWindow::onMouse " << Vec2i(x, y) << endl;
    if (auto m = getMouse()) m->motion(x, y, 0);
}

void VRGlutWindow::onKeyboard(int c, int s, int x, int y) {
    if (auto k = getKeyboard()) k->keyboard(c, s, x, y);
}

void VRGlutWindow::onKeyboard_special(int c, int s, int x, int y) {
    if (auto k = getKeyboard()) k->keyboard_special(c, s, x, y);
}

void VRGlutWindow::onDisplay() {
    #ifndef WITHOUT_OPENVR
    if (hmd) {
        hmd->render();
    }
#endif
    auto webxr = dynamic_pointer_cast<VRWebXR>( VRSetup::getCurrent()->getDevice("webxr") );
    if (webxr) webxr->preRender();
    VRWindow::render();
    if (webxr) webxr->postRender();
}

void VRGlutWindow::render(bool fromThread) {
    if (fromThread) return;
    glutPostRedisplay();
    glutMainLoopEvent();
}

OSG_END_NAMESPACE;




