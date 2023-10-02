#ifndef VRIMGUISETUP_H_INCLUDED
#define VRIMGUISETUP_H_INCLUDED

#include "VRImguiUtils.h"

using namespace std;

class ImSetupManager : public ImWidget {
    private:
        bool fotomode = false;
        bool vsync = true;
        bool framesleep = true;
        int targetFPS = 60;

    public:
        ImSetupManager();
        void begin() override;
};

#endif // VRIMGUISETUP_H_INCLUDED
