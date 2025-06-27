#ifndef VRIMGUISCENERENDERING_H_INCLUDED
#define VRIMGUISCENERENDERING_H_INCLUDED

#include "VRImguiUtils.h"
#include "imWidgets/VRImguiInput.h"
#include "imWidgets/VRImguiColorPicker.h"

using namespace std;

class ImRendering {
    private:
        int bgType = 0;
        ImInput pathInput;
        ImInput pathSplash;
        ImInput extInput;
        ImColorPicker bgColor;

        bool vsync = true;
        bool framesleep = true;
        bool splash = false;
        int targetFPS = 60;

    public:
        ImRendering();
        void render();

        void setBGType(string t);
        void setBGPath(string c);
        void setBGExt(string c);
        void setShowSplash(bool b);
        void setSplashPath(string c);
};

#endif // VRIMGUISCENERENDERING_H_INCLUDED
