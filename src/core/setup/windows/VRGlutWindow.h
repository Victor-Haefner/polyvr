#ifndef VRGLUTWINDOW_H_INCLUDED
#define VRGLUTWINDOW_H_INCLUDED

#include "VRWindow.h"
#include <OpenSG/OSGGLUTWindow.h>

OSG_BEGIN_NAMESPACE;
using namespace std;


class VRGlutWindow : public VRWindow {
    private:
        GLUTWindowMTRecPtr win;
        int winID;
        VRHeadMountedDisplayPtr hmd;

    public:
        VRGlutWindow();
        ~VRGlutWindow();

        static VRGlutWindowPtr create();
        VRGlutWindowPtr ptr();

        static void initGlut();

        void render(bool fromThread = false) override;

        void save(XMLElementPtr node) override;
        void load(XMLElementPtr node) override;

        void onDisplay();
        void onMouse(int b, int s, int x, int y);
        void onMotion(int x, int y);
        void onKeyboard(int k, int s, int x, int y);
        void onKeyboard_special(int k, int s, int x, int y);
};

OSG_END_NAMESPACE;

#endif // VRGLUTWINDOW_H_INCLUDED
