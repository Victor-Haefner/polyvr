#ifndef VRIMGUISCENERENDERING_H_INCLUDED
#define VRIMGUISCENERENDERING_H_INCLUDED

#include "VRImguiUtils.h"
#include "imWidgets/VRImguiInput.h"

using namespace std;

class ImRendering {
    private:
        ImInput pathInput;
        ImInput formatInput;

    public:
        ImRendering();
        void render();
};

#endif // VRIMGUISCENERENDERING_H_INCLUDED
