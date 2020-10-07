#ifndef VRNETWORKMANAGER_H_INCLUDED
#define VRNETWORKMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <string>
#include "core/setup/devices/VRSignal.h"
#include "VRSocket.h"
#include "core/utils/VRStorage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRNetworkManager : public VRStorage {
    private:
        map<string, VRSocketPtr> sockets;

        void test();

    protected:
        void update();

    public:
        VRNetworkManager();
        virtual ~VRNetworkManager();

        VRSocketPtr getSocket(int port);
        void remSocket(string name);
        string changeSocketName(string name, string new_name);

        VRSocketPtr getSocket(string name);
        map<string, VRSocketPtr> getSockets();
};

OSG_END_NAMESPACE

#endif // VRNETWORKMANAGER_H_INCLUDED
