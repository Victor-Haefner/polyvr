#include <OpenSG/OSGGLUT.h>
#include <GL/freeglut.h>

#ifdef WIN32
#include <GL/wglext.h>
#else
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#endif

#include "core/utils/Thread.h"

#include "VRGlutEditor.h"
#include "core/utils/VROptions.h"
#include "core/utils/VRProfiler.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/png.h"

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


bool doPrintKeyEvents = false;

OSG_BEGIN_NAMESPACE;
using namespace std;

map<int, VRGlutEditor*> glutEditors;

VRGlutEditor* getCurrentEditor() {
    int gID = glutGetWindow();
    if (!glutEditors.count(gID)) return 0;
    return glutEditors[gID];
}

void onMainDisplay() { ; }
void onMainReshape(int w, int h) { auto e = getCurrentEditor(); if (e) e->on_resize_window(w,h); }
void onMainClose() { auto e = getCurrentEditor(); if (e) e->on_close_window(); }
void onMainKeyboard(unsigned char k, int x, int y) { if (doPrintKeyEvents) printf("top key down %i\n", k); auto e = getCurrentEditor(); if (e) e->onKeyboard(k, 1, x, y); }
void onMainSpecial(int k, int x, int y) { if (doPrintKeyEvents) printf("top special down %i\n", k); auto e = getCurrentEditor(); if (e) e->onKeyboard_special(k, 1, x, y); }
void onMainKeyboardUp(unsigned char k, int x, int y) { if (doPrintKeyEvents) printf("top key up %i\n", k); auto e = getCurrentEditor(); if (e) e->onKeyboard(k, 0, x, y); }
void onMainSpecialUp(int k, int x, int y) { if (doPrintKeyEvents) printf("top special up %i\n", k); auto e = getCurrentEditor(); if (e) e->onKeyboard_special(k, 0, x, y); }

void onUIDisplay() { auto e = getCurrentEditor(); if (e) e->on_ui_display(); }
void onGLDisplay() { auto e = getCurrentEditor(); if (e) e->on_gl_display(); }
void onUIReshape(int w, int h) { auto e = getCurrentEditor(); if (e) e->on_ui_resize(w, h); }

void onPopupDisplay() { auto e = getCurrentEditor(); if (e) e->on_popup_display(); }
void onPopupReshape(int w, int h) { auto e = getCurrentEditor(); if (e) e->on_popup_resize(w, h); }
void onPopupClose() { auto e = getCurrentEditor(); if (e) e->on_popup_close(); }

// callbacks for GL view
void glutEResize(int w, int h) { auto e = getCurrentEditor(); if (e) e->on_gl_resize(w, h); }
void glutEMouse(int b, int s, int x, int y) { auto e = getCurrentEditor(); if (e) e->onMouse(b ,s ,x ,y); }
void glutEMotion(int x, int y) { auto e = getCurrentEditor(); if (e) e->onMotion(x, y); }
void glutEKeyboard(unsigned char k, int x, int y) { if (doPrintKeyEvents) printf("gl key down %i\n", k); auto e = getCurrentEditor(); if (e) e->onKeyboard(k, 1, x, y); }
void glutESpecial(int k, int x, int y) { if (doPrintKeyEvents) printf("gl special down %i\n", k); auto e = getCurrentEditor(); if (e) e->onKeyboard_special(k, 1, x, y); }
void glutEKeyboardUp(unsigned char k, int x, int y) { if (doPrintKeyEvents) printf("gl key up %i\n", k); auto e = getCurrentEditor(); if (e) e->onKeyboard(k, 0, x, y); }
void glutESpecialUp(int k, int x, int y) { if (doPrintKeyEvents) printf("gl special up %i\n", k); auto e = getCurrentEditor(); if (e) e->onKeyboard_special(k, 0, x, y); }

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
    if (VRHeadMountedDisplay::checkDeviceAttached()) {
        cout << "HMD (glut editor): init system" << endl;
        hmd = VRHeadMountedDisplay::create();
    }
#endif

    initGlut();

    cout << " Glut create editor" << endl;
    winTop = GlutWindow::create("PolyVR", 0, 0, -1, -1);
    glutEditors[winTop->winID] = this;
    glutDisplayFunc( onMainDisplay );
    glutReshapeFunc( onMainReshape );
    glutCloseFunc( onMainClose );
    glutKeyboardFunc( onMainKeyboard );
    glutSpecialFunc( onMainSpecial );
    glutKeyboardUpFunc( onMainKeyboardUp );
    glutSpecialUpFunc( onMainSpecialUp );

    initGlutExtensions(); // just after top level window creation
    maximizeWindow();

    IconList iconList;
    iconList.load("ressources/gui/logo_icon.png");
    iconList.apply();

    testGLCapabilities();

    /** IDE Window **/
    winUI = winTop->createSubWindow("ui", 0, 0, width, height);
    winUI->enableVSync(false);
    glutEditors[winUI->winID] = this;
    glutDisplayFunc( onUIDisplay );
    glutReshapeFunc( onUIReshape );

    //VRGuiManager::trigger("initGLEditor"); // TODO: use this signal to trigger initImgui below!
    VRGuiManager::get()->initImgui();
    VRGuiSignals::get()->addResizeCallback("glAreaResize", [&](int x, int y, int w, int h){ resizeGLWindow(x, y, w, h); return true; } );
    signal = [&](string name, map<string,string> opts) -> bool { return VRGuiManager::trigger(name,opts); };
    resizeSignal = [&](string name, int x, int y, int w, int h) -> bool { return VRGuiManager::triggerResize(name,x,y,w,h); };

    startGrabShiftTab();

    /** OpenSG Window **/
    winGL = winTop->createSubWindow("ugl", 0,0,width*0.6, height*0.6);
    glutPopWindow();
    glutEditors[winGL->winID] = this;

    cout << " Glut create window" << endl;
    cout << "  window ID: " << winGL->winID << endl;
    GLUTWindowMTRecPtr win = GLUTWindow::create();
    _win = win;
    win->setGlutId(winGL->winID);
    win->setSize(width, height);
    cout << "  init OpenSG GLUT window" << endl;
    win->init();
#ifndef WITHOUT_OPENVR
    if (hmd) {
        cout << "HMD (glut editor): init hmd" << endl;
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
    mgr->addCallback("ui_open_popup", [&](VRGuiSignals::Options o) { openPopupWindow(o["name"], o["title"], toInt(o["width"]), toInt(o["height"])); return true; } );
    mgr->addCallback("ui_close_popup", [&](VRGuiSignals::Options o) { closePopupWindow(); return true; } );
    mgr->addCallback("ui_toggle_popup", [&](VRGuiSignals::Options o) { togglePopupWindow(o["name"], o["title"], toInt(o["width"]), toInt(o["height"])); return true; }, true );
    mgr->addCallback("set_editor_fullscreen", [&](VRGuiSignals::Options o) { setFullscreen(toBool(o["fullscreen"])); return true; }, true );
    mgr->addCallback("uiGrabFocus", [&](VRGuiSignals::Options o) { getsFocus(winUI->winID); return true; } );
    mgr->addCallback("ui_toggle_vsync", [&](VRGuiSignals::Options o) { enableVSync(toBool(o["active"])); return true; } );
    mgr->addCallback("relayedImguiKeySignal", [&](VRGuiSignals::Options o) { handleRelayedKey(toInt(o["key"]), toInt(o["state"]), false); return true; } );
    mgr->addCallback("relayedImguiSpecialKeySignal", [&](VRGuiSignals::Options o) { handleRelayedKey(toInt(o["key"]), toInt(o["state"]), true); return true; } );
    mgr->addCallback("onSaveSnapshot", [&](VRGuiSignals::Options o) { saveSnapshot(o["path"]); return true; } );

    bool fullscreen = VROptions::get()->getOption<bool>("fullscreen");
    if (fullscreen) setFullscreen(true);

    bool maximized = VROptions::get()->getOption<bool>("maximized");
    if (maximized) setMaximized(true);
}

VRGlutEditor::~VRGlutEditor() {
    cleanupGlutExtensions();
    winGL.reset();
    winUI.reset();
    winTop.reset();
    win = NULL;
    glutEditors.clear();
}

VRGlutEditorPtr VRGlutEditor::ptr() { return static_pointer_cast<VRGlutEditor>( shared_from_this() ); }
VRGlutEditorPtr VRGlutEditor::create() { return VRGlutEditorPtr(new VRGlutEditor() ); }

void VRGlutEditor::setTitle(string title) {
    VRWindow::setTitle(title);
    winTop->setTitle(title);
}

void VRGlutEditor::setIcon(string path) {
    iconPath = path;
    VRWindow::setIcon(iconPath);
    setWindowIcon(iconPath, false);
    setWindowIcon(iconPath, true);
}

int VRGlutEditor::getCurrentWinID() { return glutGetWindow(); }
void VRGlutEditor::setCurrentWinID(int i) { glutSetWindow(i); }

int VRGlutEditor::getWinID(CONTEXT c) {
    if (c == TOP) return winTop->winID;
    if (c == SCENE) return winGL->winID;
    if (c == IMGUI) return winUI->winID;
    if (c == POPUP) return winPopup->winID;
    return -1;
}

void VRGlutEditor::onMain_Keyboard_special(int k) {
    //cout << " VRGlutEditor::onMain_Keyboard_special " << k << endl;
}

void VRGlutEditor::setMaximized(bool b) {
    Vec2i screenSize = GlutWindow::getScreenSize();

    if (b && !fullscreen) {
        cout << " glut maximize!" << endl;
        resizeGLWindow(0, 0, screenSize[0], screenSize[1]);
        maximized = b;

        winUI->activate();
        glutHideWindow();

        winTop->activate();
        glutPositionWindow(0, 0);
        glutReshapeWindow(screenSize[0], screenSize[1]);
        maximizeWindow();
    }
    else {
        cout << " glut unmaximize!" << endl;
        maximized = b;
        on_resize_window(screenSize[0], screenSize[1]);

        winUI->activate();
        glutShowWindow();
    }
}

void VRGlutEditor::setFullscreen(bool b) {
    Vec2i screenSize = GlutWindow::getScreenSize();

    if (b && !maximized) {
        cout << " glut enter fullscreen!" << endl;
        resizeGLWindow(0,0,screenSize[0], screenSize[1]);
        fullscreen = b;

        winUI->activate();
        glutHideWindow();

        winTop->activate();
        glutFullScreen(); // needs to go last to work
    } else {
        cout << " glut exit fullscreen!" << endl;
        fullscreen = b;
        on_resize_window(screenSize[0], screenSize[1]);

        winUI->activate();
        glutShowWindow();

        winTop->activate();
        glutLeaveFullScreen(); // needs to go last to work
    }
}

bool doShutdown = false;

void VRGlutEditor::on_close_window() {
    if (winTop) winTop->winID = -1;
    if (winUI) winUI->winID = -1;
    if (winGL) winGL->winID = -1;
    doShutdown = true;
    signal("glutCloseWindow", {});
}

void VRGlutEditor::on_popup_close() {
    if (winPopup) winPopup->winID = -1;
    winPopup.reset();
}

void VRGlutEditor::togglePopupWindow(string name, string title, int width, int height) {
    if (winPopup && winPopup->name == name) closePopupWindow();
    else openPopupWindow(name, title, width, height);
}

void VRGlutEditor::closePopupWindow() {
    if (!winPopup) return;
    cout << "close popup " << winPopup->name << " (" << winPopup->winID << ")" << endl;
    winPopup.reset();
}

void VRGlutEditor::openPopupWindow(string name, string title, int width, int height) {
    if (winPopup) { closePopupWindow(); return; } // TODO

    cout << "open popup " << name << endl;

    Vec2i pos  = winTop->getPosition();
    Vec2i sizeMain = winTop->getSize();
    Vec2i posPopup = pos + (sizeMain - Vec2i(width, height)) * 0.5;

    winPopup = GlutWindow::create(name, posPopup[0], posPopup[1], width, height);
    initGlutDialogExtensions(title);
    winPopup->enableVSync(false);
    glutEditors[winPopup->winID] = this;

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
    glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

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

void VRGlutEditor::getsFocus(int wID) {
    focusedWinID = wID;
    string winName = "Unknown";
    if (winGL && wID == winGL->winID) winName = "GL";
    if (winUI && wID == winUI->winID) winName = "uiMain";
    if (winPopup && wID == winPopup->winID) winName = "uiPopup";
    uiSignal("setWindowFocus", {{"winID",toString(wID)}, {"winName", winName}});
}

void VRGlutEditor::onMouse(int b, int s, int x, int y) {
    getsFocus(winGL->winID);

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
    //cout << " VRGlutEditor::onKeyboard " << c << " " << s << " " << x << " " << y << endl;
    if (s == 0 && c == 27) if (fullscreen) setFullscreen(false); // ESC
    if (focusedWinID != winGL->winID) { if (signal) signal("relayedKeySignal", {{"key",toString(c)},{"state",toString(s)}}); }
    else if (auto k = getKeyboard()) k->keyboard(c, s, x, y, 1);
}

void VRGlutEditor::onKeyboard_special(int c, int s, int x, int y) {
    //cout << " VRGlutEditor::onKeyboard_special " << c << " " << s << " " << x << " " << y << endl;
    if (s == 0 && c == 11) setFullscreen(!fullscreen); // F11
    if (focusedWinID != winGL->winID) { if (signal) signal("relayedSpecialKeySignal", {{"key",toString(c)},{"state",toString(s)}}); }
    else if (auto k = getKeyboard()) k->keyboard_special(c, s, x, y, 1);
}

void VRGlutEditor::handleRelayedKey(int key, int state, bool special) {
    //cout << "handleRelayedKey, spacial? " << special << ", key: " << key << ", state: " << state << ", has focus? " << glViewFocussed << endl;
    if (focusedWinID != winGL->winID) return;
    auto k = getKeyboard();
    if (!k) return;
    if (special) k->keyboard_special(key, state, 0, 0, 1);
    else k->keyboard(key, state, 0, 0, 1);
}

void VRGlutEditor::render(bool fromThread) {
    if (fromThread || doShutdown) return;

    glutMainLoopEvent();
    glutMainLoopEvent();
    glutMainLoopEvent();

    on_ui_display();
    on_popup_display();
    on_gl_display();

    // swap buffers
    winUI->activate();
    glutSwapBuffers();
    if (winPopup) {
        winPopup->activate();
        glutSwapBuffers();
    }
}

void VRGlutEditor::forceGLResize(int w, int h) { // TODO
    ;
}

void VRGlutEditor::enableVSync(bool b) {
    int current = glutGetWindow();
    if (winGL) { winGL->enableVSync(b); }
    if (winUI) { winUI->enableVSync(false); }
    if (winPopup) { winPopup->enableVSync(false); }
    glutSetWindow(current);
}

void VRGlutEditor::on_gl_resize(int w, int h) {
    //cout << "  Glut::on_gl_resize " << w << ", " << h << endl;
    if (!winGL) return;
    winGL->getSize();// calling this somehow magically fixes the resize glitches..
    if (resizeSignal) resizeSignal( "glutResizeGL", 0,0,w,h ); // tell imgui the new size
}

void VRGlutEditor::resizeGLWindow(int x, int y, int w, int h) { // glArea.surface
    if (fullscreen || maximized || !winGL) return;
    //cout << "     Glut::updateGLWindow " << x << ", " << y << ", " << w << ", " << h << endl;
    winGL->activate();
    glutPositionWindow(x, y);
    resize(w, h);
    glutReshapeWindow(w, h);
}

void VRGlutEditor::on_resize_window(int w, int h) { // resize top window
    if (!winUI) return;
    winUI->activate();
    glutReshapeWindow(w,h);
    if (maximized) {
        winGL->activate();
        glutPositionWindow(0, 0);
        resize(w, h);
        glutReshapeWindow(w, h);
    } else if (resizeSignal) resizeSignal("glutResize", 0,0,w,h);
}

void VRGlutEditor::saveSnapshot(string path) {
    if (!exists(getFolderName(path))) return;
    if (!winGL) return;
    winGL->activate();
    int w = 400;
    int h = 300;
    float a = h/float(w);

    Vec2i s = winGL->getSize();
    int S = min(s[0], s[1]);
    int Sa = S*a;
    int u = max(0.0, s[0]*0.5 - S*0.5);
    int v = max(0.0, s[1]*0.5 - Sa*0.5);

    int Nc = 3;
    VRImage img;
    img.setup(S, Sa, Nc);
    glReadBuffer(GL_FRONT); // alternative: GL_BACK
    glReadPixels(u, v, S, Sa, GL_RGB, GL_UNSIGNED_BYTE, &img.pixels[0]);

    img.resize(w,h);
    writePNG(path, img);
}

void VRGlutEditor::on_gl_display() {
    //cout << "  Glut::on_gl_display " << endl;
    if (!winGL) return;
    //if (VRGlobals::CURRENT_FRAME%20 != 0) return; // power saving mode
    auto profiler = VRProfiler::get();

    int pID1 = profiler->regStart("glut editor gl display");
    winGL->getSize(); // calling this somehow magically fixes the resize glitches..
    //if (signal) signal( "glutRenderGL", {} );
#ifndef WITHOUT_OPENVR
    if (hmd) {
        hmd->render();
    }
#endif
    VRWindow::render();
    profiler->regStop(pID1);
}

void VRGlutEditor::on_ui_display() {
    //cout << "  Glut::on_ui_display " << winUI << endl;
    if (!winUI) return;
    auto profiler = VRProfiler::get();

    int pID1 = profiler->regStart("glut editor ui display");
    /*auto doRender = [&]() {
        winUI->activate();
        if (signal) signal( "glutRenderUI", {} );
        glutSwapBuffers();
    };
    thread t1(doRender);
    t1.join();*/
    winUI->activate();
    if (signal) signal( "glutRenderUI", {} );
    //glutSwapBuffers();
    profiler->regStop(pID1);
}

void VRGlutEditor::on_ui_resize(int w, int h) {
    cout << "  Glut::on_ui_resize " << w << ", " << h << endl;
    if (!winUI) return;
    winUI->activate();
    if (resizeSignal) resizeSignal( "glutResize", 0,0,w,h );
}

void VRGlutEditor::on_popup_display() {
    //cout << "  Glut::on_popup_display " << winUI->winID << endl;
    if (!winPopup) return;
    auto profiler = VRProfiler::get();

    int pID1 = profiler->regStart("glut editor ui dialog display");
    winPopup->activate();
    if (signal) signal( "glutRenderPopup", {{"name",winPopup->name}} ); // may close window
    //if (winPopup) glutSwapBuffers();
    profiler->regStop(pID1);
}

void VRGlutEditor::on_popup_resize(int w, int h) {
    cout << "  Glut::on_ui_resize " << w << ", " << h << endl;
    if (!winPopup) return;
    winPopup->activate();
    if (resizeSignal) resizeSignal( "glutResizePopup", 0,0,w,h );
}

OSG_END_NAMESPACE;




