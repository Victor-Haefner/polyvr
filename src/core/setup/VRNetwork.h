#ifndef VRNETWORK_H_INCLUDED
#define VRNETWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/setup/VRSetupFwd.h"
#include "core/utils/VRManager.h"

#include <map>
#include <vector>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRNetworkSlave : public VRName {
    private:
        void update();

        string stat_multicast;
        string stat;
        VRNetworkNodePtr node;

        string display = ":0.0";
        string connection_type = "Multicast";
        int port = 3000;
        int startupDelay = 10;
        bool fullscreen = true;
        bool active_stereo = false;
        bool autostart = false;

    public:
        VRNetworkSlave(string name);
        ~VRNetworkSlave();

        static VRNetworkSlavePtr create(string name = "Slave");

        void setup(VRStorageContextPtr context);

        void setNode(VRNetworkNodePtr n);
        void set(string ct, bool fs, bool as, bool au, string a, int p, int d);

        void start();
        void stop();

        string getStatMulticast();
        string getStat();

        string getDisplay();
        string getConnectionType();
        bool getFullscreen();
        bool getActiveStereo();
        bool getAutostart();
        int getPort();
        int getStartupDelay();

        void setDisplay(string a);
        void setConnectionType(string ct);
        void setFullscreen(bool b);
        void setActiveStereo(bool b);
        void setAutostart(bool b);

        string getConnectionIdentifier();
};

class VRNetworkNode : public VRManager<VRNetworkSlave>, public std::enable_shared_from_this<VRNetworkNode> {
    private:
        string address = "localhost";
        string user = "user";
        string slavePath = "";

        string stat_node = "none";
        string stat_ssh = "none";
        string stat_ssh_key = "none";
        string stat_path = "none";

        bool isLocal();
        void initSlaves();
        void update();

    public:
        VRNetworkNode(string name);
        ~VRNetworkNode();

        static VRNetworkNodePtr create(string name = "Node");
        VRNetworkNodePtr ptr();

        void setup(VRStorageContextPtr context);

        virtual VRNetworkSlavePtr add(string name = "");

        string getAddress();
        string getUser();
        string getSlavePath();
        string getStatNode();
        string getStatSSH();
        string getStatSSHkey();
        string getStatPath();

        void setAddress(string);
        void setUser(string);
        void setSlavePath(string);
        void set(string a, string u, string p);

        bool hasFile(string path);
        void distributeKey();
        string execCmd(string cmd, bool read = true);
        void stopSlaves();
};

class VRNetwork : public VRManager<VRNetworkNode> {
    private:

    public:
        VRNetwork();
        ~VRNetwork();

        void stopSlaves();
};

OSG_END_NAMESPACE;

#endif // VRNETWORK_H_INCLUDED
