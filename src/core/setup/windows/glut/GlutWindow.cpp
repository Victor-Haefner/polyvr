#include "GlutWindow.h"

#include <cstring>
#include <GL/freeglut.h>

#ifdef WIN32
#include <GL/wglext.h>
#else
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#endif

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


GlutWindow::GlutWindow(string name) : name(name) {
    ;
}

GlutWindow::~GlutWindow() {}

GlutWindowPtr GlutWindow::create(string name) {
    auto win = GlutWindowPtr( new GlutWindow(name) );
    win->setupAsTop();
    return win;
}

GlutWindowPtr GlutWindow::ptr() { return static_pointer_cast<GlutWindow>(shared_from_this()); }

void GlutWindow::setupAsTop() {
    winID = glutCreateWindow(name.c_str());
}

GlutWindowPtr GlutWindow::createSubWindow(string name, int x0, int y0, int width, int height) {
    activate();
    auto win = GlutWindowPtr( new GlutWindow(name) );
    win->winID = glutCreateSubWindow(winID, x0, y0, width, height);
    return win;
}

void GlutWindow::activate() { glutSetWindow(winID); }

void GlutWindow::enableVSync(bool b) {
    int swapInterval = b ? 1 : 0;
    activate();
    setSwapInterval(swapInterval);
}

