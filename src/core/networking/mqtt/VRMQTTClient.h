#ifndef VRMQTTClient_H_INCLUDED
#define VRMQTTClient_H_INCLUDED

#include "../VRNetworkingFwd.h"
#include "core/utils/VRFunctionFwd.h"

#include <vector>
#include <OpenSG/OSGConfig.h>
#include "core/utils/VRMutex.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMQTTClient : public std::enable_shared_from_this<VRMQTTClient> {
    public:
        struct Data;

    private:
        Data* data = 0;

    public:
        VRMQTTClient();
        ~VRMQTTClient();

        static VRMQTTClientPtr create();
        VRMQTTClientPtr ptr();

        void connect(string address, string sub_topic, string pub_topic);
};

OSG_END_NAMESPACE;

#endif // VRMQTTClient_H_INCLUDED
