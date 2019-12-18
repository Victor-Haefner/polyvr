#ifndef VRMULTIWINDOW_H_INCLUDED
#define VRMULTIWINDOW_H_INCLUDED

#include "VRWindow.h"
#include <OpenSG/OSGMultiDisplayWindow.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRThread;

class VRMultiWindow : public VRWindow {
    private:
        vector<string> servers;
        string connection_type ="Multicast";
        MultiDisplayWindowMTRecPtr win;
        int Nx = 1;
        int Ny = 1;
        int state = INITIALIZING;
        int tries = 0;

        void render(bool fromThread = false);
        void sync(bool fromThread = false);

    public:
        VRMultiWindow();
        ~VRMultiWindow();

        static VRMultiWindowPtr create();
        VRMultiWindowPtr ptr();

        enum STATE {
            NO_CONNECTION = 0,
            INITIALIZING = 1,
            CONNECTING = 2,
            JUSTCONNECTED = 3,
            CONNECTED = 4,
            RENDERING = 5
        };

        void initialize();
        bool init_win(const std::string &msg, const std::string &server, Real32 progress);

        void addServer(string server);
        void setServer(int x, int y, string);
        string getServer(int x, int y);
        void setConnectionType(string ct);
        string getConnectionType();

        void setNTiles(int x, int y);
        int getNXTiles();
        int getNYTiles();
        int getState();
        string getStateString();

        void reset();

        void save(XMLElementPtr node);
        void load(XMLElementPtr node);
};

OSG_END_NAMESPACE;

#endif // VRMULTIWINDOW_H_INCLUDED
