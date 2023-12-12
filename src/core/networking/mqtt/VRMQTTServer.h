#ifndef VRMQTTServer_H_INCLUDED
#define VRMQTTServer_H_INCLUDED

#include "../VRNetworkingFwd.h"
#include "core/utils/VRFunctionFwd.h"

#include <vector>
#include <functional>
#include <OpenSG/OSGConfig.h>
#include "core/utils/VRMutex.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMQTTServer : public std::enable_shared_from_this<VRMQTTServer> {
    public:
        struct Data;

    private:
        Data* data = 0;
        function<string(string)> onMsg;

    public:
        VRMQTTServer();
        ~VRMQTTServer();

        static VRMQTTServerPtr create();
        VRMQTTServerPtr ptr();

        void listen(int port);
        void close();
        void onMessage( function<string(string)> cb );
};

OSG_END_NAMESPACE;

#endif // VRMQTTServer_H_INCLUDED
