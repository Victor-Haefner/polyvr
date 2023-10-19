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

        bool showDisplay = false;
        bool showWindow = false;
        bool showEditorWindow = false;
        bool showLocalWindow = false;
        bool showRemoteWindow = false;
        bool showViewport = false;
        bool showVRPN = false;
        bool showVRPNTracker = false;
        bool showART = false;
        bool showARTDevice = false;
        bool showDevice = false;
        bool showMultitouch = false;
        bool showLeap = false;
        bool showHaptics = false;
        bool showNode = false;
        bool showSlave = false;
        bool showScript = false;

        // remote window
        string remoteWinState;
        int winConnType = 0;
        Im_Vector NxNy;
        int Nx = 1;
        int Ny = 1;
        vector<string> serverIDs;

        // VRPN
        bool vrpnActive = 0;
        bool vrpnTestServer = 0;
        bool vrpnVerbose = 0;

        // slave
        int slaveConnType = 0;
        bool slaveAutostart = 0;
        bool slaveActStereo = 0;
        bool slaveFullscreen = 0;

        void hideAll();
        void treeAppend(string ID, string label, string type, string parent);
        void selectWindow(map<string,string> o);

    public:
        ImSetupManager();
        void begin() override;

        void updateSetupsList(string s);
};

#endif // VRIMGUISETUP_H_INCLUDED
