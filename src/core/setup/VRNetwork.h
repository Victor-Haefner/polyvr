#ifndef VRNETWORK_H_INCLUDED
#define VRNETWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/setup/VRSetupFwd.h"
#include "core/utils/VRManager.h"
#include "core/networking/VRNetworkingFwd.h"

#include <map>
#include <vector>
#include "core/utils/Thread.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRNetworkSlave : public VRName {
    private:
        void update();

        string stat_multicast;
        string stat;
        VRNetworkNodePtr node;

        string geometry = "512x512+0+0";
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
        string getGeometry();
        vector<string> getAvailableDisplays();

        void set(string ct, bool fs, bool as, bool au, string a, int p, int d, string g);
        void setPort(int p);
        void setDelay(int d);
        void setDisplay(string a);
        void setConnectionType(string ct);
        void setFullscreen(bool b);
        void setActiveStereo(bool b);
        void setAutostart(bool b);
        void setGeometry(string g);

        string getConnectionIdentifier();
};

class VRNetworkNode : public VRManager<VRNetworkSlave>, public std::enable_shared_from_this<VRNetworkNode> {
    private:
        VRSSHSessionPtr ssh;

        string address = "localhost";
        string user = "user";
        string slavePath = "";

        // local os if connecting to localhost
        #ifdef _WIN32
        string os = "win";
        #else
        string os = "nix";
        #endif

        string stat_node = "none";
        string stat_ssh = "none";
        string stat_ssh_key = "none";
        string stat_path = "none";

        ::Thread* initThread = 0;

        void openSSHSession();
        void initSlaves();
        void update();

    public:
        VRNetworkNode(string name);
        ~VRNetworkNode();

        static VRNetworkNodePtr create(string name = "Node");
        VRNetworkNodePtr ptr();

        void setup(VRStorageContextPtr context);
        void joinInitThread();

        virtual VRNetworkSlavePtr add(string name = "") override;

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

        bool isLocal();
        string getRemoteOS();
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

        void joinInitThreads();
        void stopSlaves();
        VRNetworkNodePtr getNode(string name);
        VRNetworkSlavePtr getSlave(string name);

        void remNode(string node);
        void remSlave(string slave);
};

OSG_END_NAMESPACE;

#endif // VRNETWORK_H_INCLUDED
