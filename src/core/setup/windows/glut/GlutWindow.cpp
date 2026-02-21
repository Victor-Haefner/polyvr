#include "GlutWindow.h"

#include <GL/freeglut.h>

#ifdef WIN32
#include <GL/wglext.h>
#else
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#endif

using namespace OSG;

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
    glutSetWindow(winID);
    auto win = GlutWindowPtr( new GlutWindow(name) );
    win->winID = glutCreateSubWindow(winID, x0, y0, width, height);
    return win;
}
