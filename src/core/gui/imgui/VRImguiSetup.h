#ifndef VRIMGUISETUP_H_INCLUDED
#define VRIMGUISETUP_H_INCLUDED

#include "VRImguiUtils.h"
#include "imWidgets/VRImguiTreeview.h"
#include "imWidgets/VRImguiVector.h"
#include "imWidgets/VRImguiCombo.h"

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

        // view
        Im_Vector viewPosition;
        Im_Vector viewSize;
        bool viewStereo = 0;
        bool viewEyesInverted = 0;
        bool viewActiveStereo = 0;
        bool viewProjection = 0;
        bool viewMirror = 0;
        ImInput eyeSeparation;
        vector<string> view_users;
        int current_view_user = 0;
        Im_Vector viewProjUser;
        Im_Vector viewProjCenter;
        Im_Vector viewProjNormal;
        Im_Vector viewProjUp;
        Im_Vector viewProjSize;
        Im_Vector viewProjShear;
        Im_Vector viewProjWarp;
        Im_Vector viewMirrorPos;
        Im_Vector viewMirrorNorm;

        // window
        bool windowActive = false;
        Im_Vector windowSize;
        ImCombo windowMSAA;
        ImCombo windowMouse;
        ImCombo windowMultitouch;
        ImCombo windowKeyboard;
        string windowTitle;
        string windowIcon;

        // remote window
        string remoteWinState;
        ImCombo winConnectionType;
        Im_Vector NxNy;
        int Nx = 1;
        int Ny = 1;
        vector<string> serverIDs;

        // ART
        bool artActive = 0;
        int artPort = 0;
        Im_Vector artOffset;
        Im_Vector artAxis;
        string artID;

        // VRPN
        bool vrpnActive = 0;
        bool vrpnTestServer = 0;
        bool vrpnVerbose = 0;

        // node
        string nodeAddress;
        string nodeUser;
        string nodeSlave;
        string nodeStatus;
        string nodeSshStatus;
        string nodeSshKeyStatus;
        string nodePathStatus;

        // slave
        ImCombo slaveConnectionType;
        ImCombo slaveSystemScreens;
        bool slaveAutostart = 0;
        bool slaveActiveStereo = 0;
        bool slaveFullscreen = 0;
        string slaveConnetionID;
        string slaveMulticast;
        string slaveStatus;
        string slaveDisplay;
        string slavePort;
        string slaveDelay;
        string slaveGeometry;

        void hideAll();
        void treeAppend(string ID, string label, string type, string parent);
        void selectView(map<string,string> o);
        void selectWindow(map<string,string> o);
        void selectMultiWindow(map<string,string> o);
        void selectNode(map<string,string> o);
        void selectSlave(map<string,string> o);
        void selectART(map<string,string> o);
        void selectARTDevice(map<string,string> o);

        void setWindowState(string window, string state);

    public:
        ImSetupManager();
        void begin() override;

        void updateSetupsList(string s);
        void updateViewTrackersList(string s);
};

#endif // VRIMGUISETUP_H_INCLUDED
