#ifndef VRIMGUIMANAGER_H_INCLUDED
#define VRIMGUIMANAGER_H_INCLUDED

#include <map>
#include <string>
#include <functional>

#include "VRImguiEditor.h"

using namespace std;

class VRImguiManager {
    private:
        Imgui imgui;

    public:
        VRImguiManager();
        ~VRImguiManager();

        void setupCallbacks();
        void initImgui();
};

#endif // VRIMGUIMANAGER_H_INCLUDED
