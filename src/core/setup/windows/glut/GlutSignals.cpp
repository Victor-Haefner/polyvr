#include "GlutSignals.h"
#include "GlutWindow.h"

#include <map>
#include <GL/freeglut.h>

using namespace OSG;

bool verbose = false;

map<int, GlutSignalsWeakPtr> glutSignalsMap;

GlutSignalsPtr getCurrentSignals() {
    int winID = GlutWindow::getActiveID();
    if (!glutSignalsMap.count(winID)) return 0;
    return glutSignalsMap[winID].lock();
}

GlutSignals::GlutSignals(string name) : name(name) {}
GlutSignals::~GlutSignals() {}

GlutSignalsPtr GlutSignals::create(string name) { return GlutSignalsPtr( new GlutSignals(name) ); }
GlutSignalsPtr GlutSignals::ptr() { return static_pointer_cast<GlutSignals>(shared_from_this()); }

void onGlutDisplay()                                 { auto s = getCurrentSignals(); if (s) s->onDisplay();               }
void onGlutReshape(int w, int h)                     { auto s = getCurrentSignals(); if (s) s->onReshape(w,h);            }
void onGlutClose()                                   { auto s = getCurrentSignals(); if (s) s->onClose();                 }
void onGlutKeyboard(unsigned char c, int x, int y)   { auto s = getCurrentSignals(); if (s) s->onKeyboard(c, 1, 0, x, y); }
void onGlutSpecial(int k, int x, int y)              { auto s = getCurrentSignals(); if (s) s->onKeyboard(k, 1, 1, x, y); }
void onGlutKeyboardUp(unsigned char c, int x, int y) { auto s = getCurrentSignals(); if (s) s->onKeyboard(c, 0, 0, x, y); }
void onGlutSpecialUp(int k, int x, int y)            { auto s = getCurrentSignals(); if (s) s->onKeyboard(k, 0, 1, x, y); }
void onGlutMouse(int b, int d, int x, int y)         { auto s = getCurrentSignals(); if (s) s->onMouse(b, d, x, y);       }
void onGlutMotion(int x, int y)                      { auto s = getCurrentSignals(); if (s) s->onMotion(x, y, 0);         }
void onGlutPassiveMotion(int x, int y)               { auto s = getCurrentSignals(); if (s) s->onMotion(x, y, 1);         }

void GlutSignals::connect(GlutWindowPtr win) {
    win->activate();
    glutSignalsMap[win->winID] = ptr();

    glutDisplayFunc      ( onGlutDisplay       );
    glutReshapeFunc      ( onGlutReshape       );
    glutCloseFunc        ( onGlutClose         );
    glutKeyboardFunc     ( onGlutKeyboard      );
    glutSpecialFunc      ( onGlutSpecial       );
    glutKeyboardUpFunc   ( onGlutKeyboardUp    );
    glutSpecialUpFunc    ( onGlutSpecialUp     );
    glutMouseFunc        ( onGlutMouse         );
    glutMotionFunc       ( onGlutMotion        );
    glutPassiveMotionFunc( onGlutPassiveMotion );
}

void GlutSignals::setDisplayCb( DisplayCallback cb ) { onDisplayCb = cb; }
void GlutSignals::setCloseCb( CloseCallback cb ) { onCloseCb = cb; }
void GlutSignals::setReshapeCb( ReshapeCallback cb ) { onReshapeCb = cb; }
void GlutSignals::setKeyboardCb( KeyboardCallback cb ) { onKeyboardCb = cb; }
void GlutSignals::setMouseCb( MouseCallback cb ) { onMouseCb = cb; }
void GlutSignals::setMotionCb( MotionCallback cb ) { onMotionCb = cb; }

void GlutSignals::onDisplay() { if (onDisplayCb) onDisplayCb(); }
void GlutSignals::onReshape(int w, int h) { if (onReshapeCb) onReshapeCb(w,h); }
void GlutSignals::onClose() { if (onCloseCb) onCloseCb(); }
void GlutSignals::onKeyboard(int k, bool d, bool s, int x, int y) { if (verbose) cout << "glut keyboard - sig " << name << ", " << k << ", " << d << ", " << s << endl; if (onKeyboardCb) onKeyboardCb(k,d,s,x,y); }
void GlutSignals::onMouse(int b, int s, int x, int y) { if (onMouseCb) onMouseCb(b,s,x,y); }
void GlutSignals::onMotion(int x, int y, bool p) { if (onMotionCb) onMotionCb(x,y,p); }



