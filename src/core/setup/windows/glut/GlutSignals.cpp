#include "GlutSignals.h"
#include "GlutWindow.h"

#include <map>
#include <GL/freeglut.h>

using namespace OSG;

map<int, GlutSignalsWeakPtr> signalsMap;

GlutSignalsPtr getCurrentSignals() {
    int winID = GlutWindow::getActiveID();
    if (!signalsMap.count(winID)) return 0;
    return signalsMap[winID].lock();
}

GlutSignals::GlutSignals() {}
GlutSignals::~GlutSignals() {}

GlutSignalsPtr GlutSignals::create() { return GlutSignalsPtr( new GlutSignals() ); }
GlutSignalsPtr GlutSignals::ptr() { return static_pointer_cast<GlutSignals>(shared_from_this()); }

void onGlutDisplay()                                 { auto s = getCurrentSignals(); if (s) s->onDisplay();               }
void onGlutReshape(int w, int h)                     { auto s = getCurrentSignals(); if (s) s->onReshape(w,h);            }
void onGlutClose()                                   { auto s = getCurrentSignals(); if (s) s->onClose();                 }
void onGlutKeyboard(unsigned char c, int x, int y)   { auto s = getCurrentSignals(); if (s) s->onKeyboard(c, 1, 0, x, y); }
void onGlutSpecial(int k, int x, int y)              { auto s = getCurrentSignals(); if (s) s->onKeyboard(k, 1, 1, x, y); }
void onGlutKeyboardUp(unsigned char c, int x, int y) { auto s = getCurrentSignals(); if (s) s->onKeyboard(c, 0, 0, x, y); }
void onGlutSpecialUp(int k, int x, int y)            { auto s = getCurrentSignals(); if (s) s->onKeyboard(k, 0, 1, x, y); }

void GlutSignals::connect(GlutWindowPtr win) {
    win->activate();
    signalsMap[win->winID] = ptr();

    glutDisplayFunc   ( onGlutDisplay    );
    glutReshapeFunc   ( onGlutReshape    );
    glutCloseFunc     ( onGlutClose      );
    glutKeyboardFunc  ( onGlutKeyboard   );
    glutSpecialFunc   ( onGlutSpecial    );
    glutKeyboardUpFunc( onGlutKeyboardUp );
    glutSpecialUpFunc ( onGlutSpecialUp  );
}

void GlutSignals::setDisplayCb( DisplayCallback cb ) { onDisplayCb = cb; }
void GlutSignals::setCloseCb( CloseCallback cb ) { onCloseCb = cb; }
void GlutSignals::setReshapeCb( ReshapeCallback cb ) { onReshapeCb = cb; }
void GlutSignals::setKeyboardCb( KeyboardCallback cb ) { onKeyboardCb = cb; }

void GlutSignals::onDisplay() { if (onDisplayCb) onDisplayCb(); }
void GlutSignals::onReshape(int w, int h) { if (onReshapeCb) onReshapeCb(w,h); }
void GlutSignals::onClose() { if (onCloseCb) onCloseCb(); }
void GlutSignals::onKeyboard(int k, bool down, bool special, int x, int y) { if (onKeyboardCb) onKeyboardCb(k,down,special,x,y); }




