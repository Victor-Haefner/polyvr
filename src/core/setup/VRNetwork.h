#ifndef VRNETWORK_H_INCLUDED
#define VRNETWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/setup/VRSetupFwd.h"

#include <map>
#include <vector>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRNetwork {
    public:
        struct Node {
            int ID = 0;
            string IP;
        };

        typedef shared_ptr<Node> NodePtr;
        typedef weak_ptr<Node> NodeWeakPtr;

    private:
        map<int, NodePtr> nodes;

    public:
        VRNetwork();
        ~VRNetwork();

        NodePtr addNetworkNode();
        vector<NodePtr> getNetworkNodes();
};

OSG_END_NAMESPACE;

#endif // VRNETWORK_H_INCLUDED
