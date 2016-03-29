#ifndef VRNETWORK_H_INCLUDED
#define VRNETWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/setup/VRSetupFwd.h"
#include "core/utils/VRManager.h"

#include <map>
#include <vector>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRNetworkNode : public VRName {
    private:
        string IP;

    public:
        VRNetworkNode(string name);
        ~VRNetworkNode();

        static VRNetworkNodePtr create(string name = "Node");
};

class VRNetwork : public VRManager<VRNetworkNode> {
    private:

    public:
        VRNetwork();
        ~VRNetwork();
};

OSG_END_NAMESPACE;

#endif // VRNETWORK_H_INCLUDED
