#ifndef VRIMGUISETUP_H_INCLUDED
#define VRIMGUISETUP_H_INCLUDED

#include "VRImguiUtils.h"

using namespace std;

class ImSetupManager : public ImWidget {
    private:
        bool fotomode = false;
        bool vsync = true;

    public:
        ImSetupManager();
        void begin() override;
};

#endif // VRIMGUISETUP_H_INCLUDED
