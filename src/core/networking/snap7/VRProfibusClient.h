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

        float toFloat(string bytes);
        short toShort(string bytes);
        int toInt(string bytes);

    public:
        VRProfinetClient();
        ~VRProfinetClient();

        static VRProfinetClientPtr create();
        VRProfinetClientPtr ptr();

        void connect(string address, int rack, int slot);
        void disconnect();
        bool isConnected();

        string read(int db, int offset, int length,string dbType = "database");
        void write(int db, int offset, string val,string dbType = "database");

        bool  readBool(int db, int pos, int bit,string dbType = "database");
        short readShort(int db, int pos,string dbType = "database");
        int   readInt(int db, int pos,string dbType = "database");
        float readFloat(int db, int pos,string dbType = "database");

        void writeBool(int db, int pos, int bit, bool val,string dbType = "database");
        void writeShort(int db, int pos, short val,string dbType = "database");
        void writeInt(int db, int pos, int val,string dbType = "database");
        void writeFloat(int db, int pos, float val,string dbType = "database");
};

OSG_END_NAMESPACE;

#endif // VRPROFIBUSClient_H_INCLUDED
