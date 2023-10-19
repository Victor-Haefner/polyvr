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
        string selected;
        int winConnType = 0;
        int slaveConnType = 0;
        bool vrpnActive = 0;
        bool vrpnTestServer = 0;
        bool vrpnVerbose = 0;
        bool slaveAutostart = 0;
        bool slaveActStereo = 0;
        bool slaveFullscreen = 0;

        void treeAppend(string ID, string label, string type, string parent);

    public:
        ImSetupManager();
        void begin() override;

        void updateSetupsList(string s);
};

#endif // VRIMGUISETUP_H_INCLUDED
