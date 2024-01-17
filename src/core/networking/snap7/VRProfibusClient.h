#ifndef VRPROFIBUSClient_H_INCLUDED
#define VRPROFIBUSClient_H_INCLUDED

#include "../VRNetworkingFwd.h"
#include "core/utils/VRFunctionFwd.h"

#include <vector>
#include <OpenSG/OSGConfig.h>
#include "core/utils/VRMutex.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRProfinetClient : public std::enable_shared_from_this<VRProfinetClient> {
    public:
        struct Data;

    private:
        Data* data = 0;

    public:
        VRProfinetClient();
        ~VRProfinetClient();

        static VRProfinetClientPtr create();
        VRProfinetClientPtr ptr();

        void connect(string address, int rack, int slot);
        void disconnect();
        bool isConnected();

        string read(int db, int offset, int length);
        void write(int db, int offset, string val);
};

OSG_END_NAMESPACE;

#endif // VRPROFIBUSClient_H_INCLUDED
