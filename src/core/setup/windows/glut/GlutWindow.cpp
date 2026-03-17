#include "GlutWindow.h"
#include "GlutSignals.h"

#include <map>
#include <cstring>
#include <iostream>
#include <GL/freeglut.h>

#ifdef WIN32
#include <GL/wglext.h>
#else
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#endif

#include "VRGlutExtensions.h"

using namespace OSG;


void setSwapInterval(int swapInterval) {
#ifdef WIN32
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
    if (wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT"))
    {
        // Enable V-Sync
        wglSwapIntervalEXT(swapInterval); // 0 for off, 1 for on
    };
#else
    Display *display = glXGetCurrentDisplay();
    GLXDrawable drawable = glXGetCurrentDrawable();

    const char *extensions = glXQueryExtensionsString(display, DefaultScreen(display));
    bool extSupported = strstr(extensions, "GLX_EXT_swap_control") != nullptr;
    bool sgiSupported = strstr(extensions, "GLX_SGI_swap_control") != nullptr;
    bool mesaSupported = strstr(extensions, "GLX_MESA_swap_control") != nullptr;

    if (extSupported) {
        typedef void (*glXSwapIntervalEXTProc)(Display*, GLXDrawable, int);
        glXSwapIntervalEXTProc glXSwapIntervalEXTptr = (glXSwapIntervalEXTProc)glXGetProcAddress((const GLubyte *)"glXSwapIntervalEXT");
        if (glXSwapIntervalEXTptr) glXSwapIntervalEXTptr(display, drawable, swapInterval); // 0 for off, 1 for on
    }

    if (sgiSupported) {
        typedef int (*glXSwapIntervalSGIProc)(int);
        glXSwapIntervalSGIProc glXSwapIntervalSGIptr = (glXSwapIntervalSGIProc)glXGetProcAddress((const GLubyte *)"glXSwapIntervalSGI");
        if (glXSwapIntervalSGIptr) glXSwapIntervalSGIptr(swapInterval); // 0 for off, 1 for on
    }

    if (mesaSupported) {
        typedef int (*glXSwapIntervalMESAProc)(unsigned int);
        glXSwapIntervalMESAProc glXSwapIntervalMESAptr = (glXSwapIntervalMESAProc)glXGetProcAddress((const GLubyte *)"glXSwapIntervalMESA");
        if (glXSwapIntervalMESAptr) glXSwapIntervalMESAptr(swapInterval); // 0 for off, 1 for on
    }
#endif
}


map<int, GlutWindowWeakPtr> glutWindowsMap;

GlutWindow::GlutWindow(string name) : name(name) {
    signals = GlutSignals::create(name+"-signals");
}

GlutWindow::~GlutWindow() {
    if (winID < 0) return;
    glutDestroyWindow(winID);
}

GlutWindowPtr GlutWindow::create(string name, int x0, int y0, int width, int height) {
    auto win = GlutWindowPtr( new GlutWindow(name) );
    win->setupAsTop(x0, y0, width, height);
    win->signals->connect(win);
    return win;
}

GlutWindowPtr GlutWindow::ptr() { return static_pointer_cast<GlutWindow>(shared_from_this()); }

GlutWindowPtr GlutWindow::getActive() {
    int winID = getActiveID();
    if (!glutWindowsMap.count(winID)) return 0;
    return glutWindowsMap[winID].lock();
}

void GlutWindow::setupAsTop(int x0, int y0, int width, int height) {
    Vec2i screenSize = getScreenSize();
    if (width  < 0) width  = screenSize[0];
    if (height < 0) height = screenSize[1];
    glutInitWindowSize(width, height);
    glutInitWindowPosition(x0, y0);
    winID = glutCreateWindow(name.c_str());
    glutWindowsMap[winID] = ptr();
}

GlutWindowPtr GlutWindow::createSubWindow(string name, int x0, int y0, int width, int height) {
    activate();
    auto win = GlutWindowPtr( new GlutWindow(name) );
    win->winID = glutCreateSubWindow(winID, x0, y0, width, height);
    win->signals->connect(win);
    glutWindowsMap[win->winID] = win;
    return win;
}

int GlutWindow::getActiveID() { return glutGetWindow(); }

void GlutWindow::setTitle(string title) {
    int w = glutGetWindow();
    activate();
    glutSetWindowTitle(title.c_str());
    glutSetWindow(w);
}

void GlutWindow::setDisplayCb( GlutSignals::DisplayCallback cb ) { signals->setDisplayCb(cb); }
void GlutWindow::setCloseCb( GlutSignals::CloseCallback cb ) { signals->setCloseCb(cb); }
void GlutWindow::setReshapeCb( GlutSignals::ReshapeCallback cb ) { signals->setReshapeCb(cb); }
void GlutWindow::setKeyboardCb( GlutSignals::KeyboardCallback cb ) { signals->setKeyboardCb(cb); }
void GlutWindow::setMouseCb( GlutSignals::MouseCallback cb ) { signals->setMouseCb(cb); }
void GlutWindow::setMotionCb( GlutSignals::MotionCallback cb ) { signals->setMotionCb(cb); }

void GlutWindow::activate() { glutSetWindow(winID); }

Vec2i GlutWindow::getScreenSize() { // static
    Vec2i S = Vec2i(glutGet(GLUT_SCREEN_WIDTH), glutGet(GLUT_SCREEN_HEIGHT));
    //cout << " screen size " << S << endl;
    return S;
}

Vec2i GlutWindow::getPosition() {
    activate();
    return Vec2i(glutGet(GLUT_WINDOW_X), glutGet(GLUT_WINDOW_Y));
}

Vec2i GlutWindow::getSize() {
    activate();
    return Vec2i(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
}

void GlutWindow::setPosition(Vec2i p) {
    activate();
    glutPositionWindow(p[0], p[1]);
}

void GlutWindow::setSize(Vec2i s) {
    activate();
    glutReshapeWindow(s[0], s[1]);
}

void GlutWindow::enableVSync(bool b) {
    int swapInterval = b ? 1 : 0;
    activate();
    setSwapInterval(swapInterval);
}

void GlutWindow::setVisible(bool b) {
    activate();
    if (b) glutShowWindow();
    else glutHideWindow();
}

void GlutWindow::setFullscreen(bool b) {
    activate();
    if (b) glutFullScreen();
    else glutLeaveFullScreen();
}

void GlutWindow::setMaximized(bool b) { // TODO: unmaximize doesnt work properly!
    activate();

    if (b) {
        unMaximizedSize = getSize();
        unMaximizedPosition = getPosition();
        //setPosition(Vec2i(0,0));
        //setSize( GlutWindow::getScreenSize() );
        maximizeWindow();
    } else {
        setPosition(unMaximizedPosition);
        setSize(unMaximizedSize);
    }
}


