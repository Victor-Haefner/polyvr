#ifndef VRIMGUISETUP_H_INCLUDED
#define VRIMGUISETUP_H_INCLUDED

#include "VRImguiUtils.h"
#include "imWidgets/VRImguiTreeview.h"
#include "imWidgets/VRImguiVector.h"

using namespace std;

class ImSetupManager : public ImWidget {
    private:
        int current_setup = 0;
        vector<string> setups;
        ImTreeview tree;

    public:
        ImSetupManager();
        void begin() override;

        void updateSetupsList(string s);
};

#endif // VRIMGUISETUP_H_INCLUDED
