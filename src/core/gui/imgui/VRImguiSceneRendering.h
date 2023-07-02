#ifndef VRIMGUISCENERENDERING_H_INCLUDED
#define VRIMGUISCENERENDERING_H_INCLUDED

#include "VRImguiUtils.h"
#include "imWidgets/VRImguiInput.h"

using namespace std;

class ImRendering {
    private:
        int bgType = 0;
        ImInput pathInput;
        ImInput extInput;
        ImVec4 color;

    public:
        ImRendering();
        void render();

        void setBGType(string t);
        void setBGColor(string c);
        void setBGPath(string c);
        void setBGExt(string c);
};

#endif // VRIMGUISCENERENDERING_H_INCLUDED
