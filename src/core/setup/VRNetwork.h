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

    public:
        VRNetworkSlave(string name);
        ~VRNetworkSlave();

        static VRNetworkSlavePtr create(string name = "Slave");
};

class VRNetworkNode : public VRManager<VRNetworkSlave> {
    private:
        string address = "localhost";
        string user = "user";

        string stat_node = "none";
        string stat_ssh = "none";
        string stat_ssh_key = "none";

        void update();

    public:
        VRNetworkNode(string name);
        ~VRNetworkNode();

        static VRNetworkNodePtr create(string name = "Node");

        string getAddress();
        string getUser();
        string getStatNode();
        string getStatSSH();
        string getStatSSHkey();

        void setAddress(string);
        void setUser(string);
        void set(string a, string u);

        void distributeKey();
};

class VRNetwork : public VRManager<VRNetworkNode> {
    private:

    public:
        VRNetwork();
        ~VRNetwork();
};

OSG_END_NAMESPACE;

#endif // VRNETWORK_H_INCLUDED
