#include <OpenSG/OSGGLUT.h>
#include <GL/freeglut.h>

#include "VRGlutEditor.h"
#include "core/utils/VROptions.h"

#include "../devices/VRMouse.h"
#include "../devices/VRKeyboard.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRWebXR.h"
#include "core/scene/VRScene.h"

#include "core/gui/VRGuiManager.h"
#include "core/gui/imgui/VRImguiManager.h"
#include "glut/VRGlutExtensions.h"

#ifndef WITHOUT_OPENVR
#include "VRHeadMountedDisplay.h"
#endif



OSG_BEGIN_NAMESPACE;
using namespace std;

map<int, VRGlutEditor*> glutEditors;

VRGlutEditor* getCurrentEditor() {
    return glutEditors[glutGetWindow()];
}

void onMainDisplay() { ; }
void onMainReshape(int w, int h) { getCurrentEditor()->on_resize_window(w,h); }
void onMainClose() { getCurrentEditor()->on_close_window(); }
void onMainKeyboard(unsigned char k, int x, int y) { /*printf("gl key down %i\n", k);*/ getCurrentEditor()->onKeyboard(k, 1, x, y); }
void onMainSpecial(int k, int x, int y) { /*printf("gl special down %i\n", k);*/ getCurrentEditor()->onKeyboard_special(k, 1, x, y); }
void onMainKeyboardUp(unsigned char k, int x, int y) { /*printf("gl key up %i\n", k);*/ getCurrentEditor()->onKeyboard(k, 0, x, y); }
void onMainSpecialUp(int k, int x, int y) { /*printf("gl special up %i\n", k);*/ getCurrentEditor()->onKeyboard_special(k, 0, x, y); }

void onUIDisplay() { getCurrentEditor()->on_ui_display(); }
void onGLDisplay() { getCurrentEditor()->on_gl_display(); }
void onUIReshape(int w, int h) { getCurrentEditor()->on_ui_resize(w, h); }

void onPopupDisplay() { getCurrentEditor()->on_popup_display(); }
void onPopupReshape(int w, int h) { getCurrentEditor()->on_popup_resize(w, h); }
void onPopupClose() { getCurrentEditor()->on_popup_close(); }

// callbacks for GL view
void glutEResize(int w, int h) { getCurrentEditor()->on_gl_resize(w, h); }
void glutEMouse(int b, int s, int x, int y) { getCurrentEditor()->onMouse(b ,s ,x ,y); }
void glutEMotion(int x, int y) { getCurrentEditor()->onMotion(x, y); }
void glutEKeyboard(unsigned char k, int x, int y) { /*printf("gl key down %i\n", k);*/ getCurrentEditor()->onKeyboard(k, 1, x, y); }
void glutESpecial(int k, int x, int y) { /*printf("gl special down %i\n", k);*/ getCurrentEditor()->onKeyboard_special(k, 1, x, y); }
void glutEKeyboardUp(unsigned char k, int x, int y) { /*printf("gl key up %i\n", k);*/ getCurrentEditor()->onKeyboard(k, 0, x, y); }
void glutESpecialUp(int k, int x, int y) { /*printf("gl special up %i\n", k);*/ getCurrentEditor()->onKeyboard_special(k, 0, x, y); }

void testGLCapabilities() {
    cout << "Check OpenGL capabilities:" << endl;
    cout << " OpenGL vendor: " << VRRenderManager::getGLVendor() << endl;
    cout << " OpenGL version: " << VRRenderManager::getGLVersion() << endl;
    cout << " GLSL version: " << VRRenderManager::getGLSLVersion() << endl;
    cout << " has geometry shader: " << VRRenderManager::hasGeomShader() << endl;
    cout << " has tesselation shader: " << VRRenderManager::hasTessShader() << endl;
}

VRGlutEditor::VRGlutEditor() {
    cout << "Glut: New Editor" << endl;
    type = "glutEditor";

#ifndef WITHOUT_OPENVR
    if (VRHeadMountedDisplay::checkDeviceAttached())
        hmd = VRHeadMountedDisplay::create();
#endif

    initGlut();

    int width = glutGet(GLUT_SCREEN_WIDTH);
    int height = glutGet(GLUT_SCREEN_HEIGHT);

    cout << " Glut create editor" << endl;
    glutInitWindowSize(width, height);
    topWin = glutCreateWindow("PolyVR");
    glutEditors[topWin] = this;
    glutDisplayFunc( onMainDisplay );
    glutReshapeFunc( onMainReshape );
    glutCloseFunc( onMainClose );
    glutKeyboardFunc( onMainKeyboard );
    glutSpecialFunc( onMainSpecial );
    glutKeyboardUpFunc( onMainKeyboardUp );
    glutSpecialUpFunc( onMainSpecialUp );

    initGlutExtensions(); // just after top level window creation

    IconList iconList;
    iconList.load("ressources/gui/logo_icon.png");
    iconList.apply();

    testGLCapabilities();

    /** IDE Window **/
    winUI = glutCreateSubWindow(topWin, 0, 0, width, height);
    glutEditors[winUI] = this;
    glutDisplayFunc( onUIDisplay );
    glutReshapeFunc( onUIReshape );

    //VRGuiManager::trigger("initGLEditor"); // TODO: use this signal to trigger initImgui below!
    VRGuiManager::get()->initImgui();
    VRGuiSignals::get()->addResizeCallback("glAreaResize", [&](int x, int y, int w, int h){ resizeGLWindow(x, y, w, h); return true; } );
    signal = [&](string name, map<string,string> opts) -> bool { return VRGuiManager::trigger(name,opts); };
    resizeSignal = [&](string name, int x, int y, int w, int h) -> bool { return VRGuiManager::triggerResize(name,x,y,w,h); };

    startGrabShiftTab();

    /** OpenSG Window **/
    glutSetWindow(topWin);
    winGL = glutCreateSubWindow(topWin, 0,0,width*0.6, height*0.6);
    glutPopWindow();
    glutEditors[winGL] = this;

    cout << " Glut create window" << endl;
    cout << "  window ID: " << winGL << endl;
    GLUTWindowMTRecPtr win = GLUTWindow::create();
    _win = win;
    win->setGlutId(winGL);
    win->setSize(width, height);
    cout << "  init OpenSG GLUT window" << endl;
    win->init();
#ifndef WITHOUT_OPENVR
    if (hmd) {
        hmd->initHMD();
    }
#endif

    glutDisplayFunc( onGLDisplay );
    glutReshapeFunc(glutEResize);
    glutKeyboardFunc(glutEKeyboard);
    glutSpecialFunc(glutESpecial);
    glutKeyboardUpFunc(glutEKeyboardUp);
    glutSpecialUpFunc(glutESpecialUp);
    glutMotionFunc(glutEMotion);
    glutPassiveMotionFunc(glutEMotion);
    glutMouseFunc(glutEMouse);
    cout << " Glut window initiated" << endl;

    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("ui_open_popup", [&](VRGuiSignals::Options o) { openPopupWindow(o["name"], toInt(o["width"]), toInt(o["height"])); return true; } );
    mgr->addCallback("ui_close_popup", [&](VRGuiSignals::Options o) { closePopupWindow(); return true; } );
    mgr->addCallback("ui_toggle_popup", [&](VRGuiSignals::Options o) { togglePopupWindow(o["name"], toInt(o["width"]), toInt(o["height"])); return true; }, true );
    mgr->addCallback("set_editor_fullscreen", [&](VRGuiSignals::Options o) { setFullscreen(toBool(o["fullscreen"])); return true; }, true );
    mgr->addCallback("uiGrabFocus", [&](VRGuiSignals::Options o) { glViewFocussed = false; return true; } );
    mgr->addCallback("ui_toggle_vsync", [&](VRGuiSignals::Options o) { enableVSync(toBool(o["active"])); return true; } );
    mgr->addCallback("relayedImguiKeySignal", [&](VRGuiSignals::Options o) { handleRelayedKey(toInt(o["key"]), toInt(o["state"]), false); return true; } );
    mgr->addCallback("relayedImguiSpecialKeySignal", [&](VRGuiSignals::Options o) { handleRelayedKey(toInt(o["key"]), toInt(o["state"]), true); return true; } );

    bool fullscreen = VROptions::get()->getOption<bool>("fullscreen");
    if (fullscreen) setFullscreen(true);
}

VRGlutEditor::~VRGlutEditor() {
    cleanupGlutExtensions();
    glutDestroyWindow(winGL);
    glutDestroyWindow(winUI);
    glutDestroyWindow(topWin);
    win = NULL;
}

VRGlutEditorPtr VRGlutEditor::ptr() { return static_pointer_cast<VRGlutEditor>( shared_from_this() ); }
VRGlutEditorPtr VRGlutEditor::create() { return VRGlutEditorPtr(new VRGlutEditor() ); }

void VRGlutEditor::onMain_Keyboard_special(int k) {
    //cout << " VRGlutEditor::onMain_Keyboard_special " << k << endl;
}

void VRGlutEditor::setFullscreen(bool b) {
    int width = glutGet(GLUT_SCREEN_WIDTH);
    int height = glutGet(GLUT_SCREEN_HEIGHT);

    if (b) {
        cout << " glut enter fullscreen!" << endl;
        resizeGLWindow(0,0,width,height);
        fullscreen = b;

        glutSetWindow(winUI);
        glutHideWindow();

        glutSetWindow(topWin);
        glutFullScreen(); // needs to go last to work
    } else {
        cout << " glut exit fullscreen!" << endl;
        fullscreen = b;
        on_resize_window(width, height);

        glutSetWindow(winUI);
        glutShowWindow();

        glutSetWindow(topWin);
        glutLeaveFullScreen(); // needs to go last to work
    }
}

void VRGlutEditor::on_close_window() { signal( "glutCloseWindow", {} ); }
void VRGlutEditor::on_popup_close() { popup = ""; winPopup = -1; }

void VRGlutEditor::togglePopupWindow(string name, int width, int height) {
    if (popup == name) closePopupWindow();
    else openPopupWindow(name, width, height);
}

void VRGlutEditor::closePopupWindow() {
    if (winPopup < 0) return;
    popup = "";
    glutDestroyWindow(winPopup);
    winPopup = -1;
}

void VRGlutEditor::openPopupWindow(string name, int width, int height) {
    popup = name;

    int screenWidth = glutGet(GLUT_SCREEN_WIDTH);
    int screenHeight = glutGet(GLUT_SCREEN_HEIGHT);

    glutInitWindowSize(width, height);
    glutInitWindowPosition((screenWidth-width)*0.5, (screenHeight-height)*0.5);
    winPopup = glutCreateWindow(name.c_str());
    glutEditors[winPopup] = this;

    glutDisplayFunc( onPopupDisplay );
    glutReshapeFunc( onPopupReshape );
    glutCloseFunc( onPopupClose );
    VRGuiManager::get()->initImguiPopup();
}

void VRGlutEditor::initGlut() {
    static bool glutInititated = false;
    if (glutInititated) return;
    glutInititated = true;
    cout << " init GLUT (VRGlutEditor)";

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

void VRGlutEditor::save(XMLElementPtr node) { VRWindow::save(node); }
void VRGlutEditor::load(XMLElementPtr node) { VRWindow::load(node); }

void VRGlutEditor::onMouse(int b, int s, int x, int y) {
    glViewFocussed = true;

    // swap mouse wheel
    if (b == 3) b = 4;
    else if (b == 4) b = 3;

    //cout << "VRGlutEditor::onMouse " << Vec4i(b, s, x, y) << endl;
    if (auto m = getMouse()) m->mouse(b, s, x, y, 1);
}

void VRGlutEditor::onMotion(int x, int y) {
    //cout << "VRGlutEditor::onMotion " << Vec2i(x, y) << endl;
    glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);
    if (auto m = getMouse()) m->motion(x, y, 1);
}

void VRGlutEditor::onKeyboard(int c, int s, int x, int y) {
    if (!glViewFocussed) { if (signal) signal("relayedKeySignal", {{"key",toString(c)},{"state",toString(s)}}); }
    else if (auto k = getKeyboard()) k->keyboard(c, s, x, y, 1);
}

void VRGlutEditor::onKeyboard_special(int c, int s, int x, int y) {
    if (s == 0 && c == 11) setFullscreen(!fullscreen);
    if (!glViewFocussed) { if (signal) signal("relayedSpecialKeySignal", {{"key",toString(c)},{"state",toString(s)}}); }
    else if (auto k = getKeyboard()) k->keyboard_special(c, s, x, y, 1);
    //cout << " VRGlutEditor::onKeyboard_special " << c << " " << s << " " << x << " " << y << endl;
}

void VRGlutEditor::handleRelayedKey(int key, int state, bool special) {
    //cout << "handleRelayedKey, spacial? " << special << ", key: " << key << ", state: " << state << ", has focus? " << glViewFocussed << endl;
    if (!glViewFocussed) return;
    auto k = getKeyboard();
    if (special) if (k) k->keyboard_special(key, state, 0, 0, 1);
    else if (k) k->keyboard(key, state, 0, 0, 1);
}

void VRGlutEditor::render(bool fromThread) {
    if (fromThread) return;
    glutSetWindow(winGL);
    glutPostRedisplay();
    glutSetWindow(winUI);
    glutPostRedisplay();
    if (winPopup >= 0) {
        glutSetWindow(winPopup);
        glutPostRedisplay();
    }

    glutMainLoopEvent();
    glutMainLoopEvent(); // call again after window reshapes
    glutMainLoopEvent(); // call again after window reshapes
}

void VRGlutEditor::forceGLResize(int w, int h) { // TODO
    ;
}

#ifdef WIN32
#include <GL/wglext.h>
#endif

void setSwapInterval(int swapInterval) { // TODO, Linux
#ifdef WIN32
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
    if (wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT"))
    {
        // Enable V-Sync
        wglSwapIntervalEXT(swapInterval); // 0 for off, 1 for on
    };
#else
#endif
}

void VRGlutEditor::enableVSync(bool b) {
    int swapInterval = b ? 1 : 0;

    int current = glutGetWindow();
    if (winGL >= 0) glutSetWindow(winGL); setSwapInterval(swapInterval);
    if (winUI >= 0) glutSetWindow(winUI); setSwapInterval(swapInterval);
    if (winPopup >= 0) glutSetWindow(winPopup); setSwapInterval(swapInterval);
    glutSetWindow(current);
}

void VRGlutEditor::on_gl_resize(int w, int h) {
    //cout << "  Glut::on_gl_resize " << w << ", " << h << endl;
    if (winGL < 0) return;
    glutSetWindow(winGL);
    int ww = glutGet(GLUT_WINDOW_WIDTH); // calling glutGet somehow magically fixes the resize glitches..
    int hh = glutGet(GLUT_WINDOW_HEIGHT);
    if (resizeSignal) resizeSignal( "glutResizeGL", 0,0,w,h ); // tell imgui the new size
}

void VRGlutEditor::resizeGLWindow(int x, int y, int w, int h) { // glArea.surface
    if (fullscreen) return;
    //cout << "     Glut::updateGLWindow " << x << ", " << y << ", " << w << ", " << h << endl;
    if (winGL < 0) return;
    glutSetWindow(winGL);
    glutPositionWindow(x, y);
    resize(w, h);
    glutReshapeWindow(w, h);
}

void VRGlutEditor::on_resize_window(int w, int h) { // resize top window
    if (winUI < 0) return;
    glutSetWindow(winUI);
    glutReshapeWindow(w,h);
    if (resizeSignal) resizeSignal("glutResize", 0,0,w,h);
}

void VRGlutEditor::on_gl_display() {
    //cout << "  Glut::on_gl_display " << endl;
    if (winGL < 0) return;
    glutSetWindow(winGL);
    int w = glutGet(GLUT_WINDOW_WIDTH); // calling glutGet somehow magically fixes the resize glitches..
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    //if (signal) signal( "glutRenderGL", {} );
#ifndef WITHOUT_OPENVR
    if (hmd) {
        hmd->render();
    }
#endif
    VRWindow::render();
}

void VRGlutEditor::on_ui_display() {
    //cout << "  Glut::on_ui_display " << winUI << endl;
    if (winUI < 0) return;
    glutSetWindow(winUI);
    if (signal) signal( "glutRenderUI", {} );
    glutSwapBuffers();
}

void VRGlutEditor::on_ui_resize(int w, int h) {
    cout << "  Glut::on_ui_resize " << w << ", " << h << endl;
    if (winUI < 0) return;
    glutSetWindow(winUI);
    if (resizeSignal) resizeSignal( "glutResize", 0,0,w,h );
}

void VRGlutEditor::on_popup_display() {
    //cout << "  Glut::on_popup_display " << winUI << endl;
    if (winPopup < 0) return;
    glutSetWindow(winPopup);
    if (signal) signal( "glutRenderPopup", {{"name",popup}} ); // may close window
    if (winPopup >= 0) glutSwapBuffers();
}

void VRGlutEditor::on_popup_resize(int w, int h) {
    cout << "  Glut::on_ui_resize " << w << ", " << h << endl;
    if (winPopup < 0) return;
    glutSetWindow(winPopup);
    if (resizeSignal) resizeSignal( "glutResizePopup", 0,0,w,h );
}

OSG_END_NAMESPACE;




