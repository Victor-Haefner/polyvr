#ifndef VRSCENESYNC_H_INCLUDED
#define VRSCENESYNC_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>

using namespace std;

OSG_BEGIN_NAMESPACE;

class PointConnection;
class RemoteAspect;

class VRSceneSync {
    private:
        PointConnection*  _connection;
        RemoteAspect*     _aspect;

        string _requestAddress;
        string _boundAddress;
        string _serviceName;
        string _connectionType;
        string _serviceGroup;
        string _interface;

        int _servicePort;
        int _serverId;

        void acceptClient();

    public:
        VRSceneSync();
        ~VRSceneSync();

        void start();
        void stop();
        void sync();

        void connect(string address, int port);

        void simpleUpdate();

        static void test();
};

OSG_END_NAMESPACE;

#endif // VRSCENESYNC_H_INCLUDED
