#include "VRSceneSync.h"

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGThread.h>
#include <OpenSG/OSGThreadManager.h>
#include <OpenSG/OSGClusterServer.h>
#include <OpenSG/OSGPointConnection.h>
#include <OpenSG/OSGConnectionFactory.h>
#include <OpenSG/OSGDgramSocket.h>
#include <OpenSG/OSGClusterWindow.h>
#include <OpenSG/OSGBinaryMessage.h>
#include <OpenSG/OSGClusterNetwork.h>
#include <OpenSG/OSGRemoteAspect.h>
#include <OpenSG/OSGTypeFactory.h>

using namespace OSG;

VRSceneSync::VRSceneSync() {}
VRSceneSync::~VRSceneSync() {}

void VRSceneSync::connect(string address, int port) {}

void VRSceneSync::acceptClient() {
    BinaryMessage  msg;
    DgramSocket    serviceSock;
    SocketAddress  addr;
    std::string    service;
    std::string    connectionType;
    UInt32         readable;
    bool           connected=false;
    std::string    address;
    bool           bound = false;

    SINFO << "Waiting for request of "
          << _serviceName
          << std::endl;

    try
    {
        if(!_requestAddress.empty())
        {
            // create connection

            _connection = ConnectionFactory::the()->createPoint(
                _connectionType);

            if(_connection)
            {
                // set interface
                _connection->setInterface(_interface);
                // bind connection
                try
                {
                    // bind to requested address
                    _boundAddress = _connection->bind(_requestAddress);
                    bound = true;
                }
                catch(...)
                {
                    SINFO << "Unable to bind, use name as symbolic "
                          << "service name"
                          << std::endl;
                }
            }
        }

        serviceSock.open();
        serviceSock.setReusePort(true);

        // join to multicast group
        if(!_serviceGroup.empty())
        {
            SocketAddress groupAddress =
                SocketAddress(_serviceGroup.c_str(),
                              _servicePort);

            if(groupAddress.isMulticast())
            {
                SINFO << "wait for request on multicast:"
                      << _serviceGroup << std::endl;

                serviceSock.bind(SocketAddress(SocketAddress::ANY,
                                               _servicePort));
                serviceSock.join(SocketAddress(groupAddress));
            }
            else
            {
                SINFO << "wait for request by broadcast:"
                      << _serviceGroup << std::endl;
                serviceSock.bind(SocketAddress(groupAddress));
            }
        }
        else
        {
            SINFO << "wait for request by broadcast" << std::endl;
            serviceSock.bind(SocketAddress(SocketAddress::ANY,
                                           _servicePort));
        }

        while(!connected)
        {
            try
            {
                if(_connection)
                    readable = serviceSock.waitReadable(.01);
                else
                    readable = true;

                if(readable)
                {
                    serviceSock.recvFrom(msg,addr);

                    service        = msg.getString();
                    connectionType = msg.getString();

                    SINFO << "Request for "
                          << service << " "
                          << connectionType
                          << std::endl;

                    if(service == _serviceName)
                    {
                        // remove old connection if typename missmaches
                        if(_connection &&
                           _connection->getType()->getName() != connectionType)
                        {
                            delete _connection;
                            _connection = NULL;
                        }

                        // try to create connection
                        if(!_connection)
                        {
                            // create connection
                            _connection =
                                ConnectionFactory::the()->createPoint(
                                    connectionType);

                            if(_connection)
                            {
                                // set interface
                                _connection->setInterface(_interface);
                                // bind connection
                                _boundAddress = _connection->bind(
                                    _requestAddress);

                                bound = true;
                            }
                            else
                            {
                                SINFO << "Unknown connection type '"
                                      << connectionType << "'" << std::endl;
                            }
                        }

                        if(_connection)
                        {
                            msg.clear    (             );
                            msg.putString(_serviceName );
                            msg.putString(_boundAddress);
                            serviceSock.sendTo(msg, addr);

                            SINFO << "Response "
                                  << connectionType << ":"
                                  << _boundAddress
                                  << std::endl;
                        }
                    }
                }
            }

            catch(SocketConnReset &e)
            {
                // ignore if there is a connection. This can happen, if
                // a client has send a request. The server has send an
                // answer meanwile the client has send a second request
                // the client gets the answer to the first request and
                // the server tries to send a second answer. The second
                // answer can not be delivered because the client has
                // closed its service port. This is a win-socket problem.

                SWARNING << e.what() << std::endl;

                // if there is no connection, then its a real problem
                if(!_connection)
                    throw;
            }
            catch(OSG_STDEXCEPTION_NAMESPACE::exception &e)
            {
                SWARNING << e.what() << std::endl;
            }
            try
            {
                // try to accept
                if(bound && _connection && _connection->acceptGroup(0.2) >= 0)
                {
                    connected = true;
                    SINFO << "Connection accepted "
                          << _boundAddress
                          << std::endl;
                }
            }

            catch(OSG_STDEXCEPTION_NAMESPACE::exception &e)
            {
                SWARNING << e.what() << std::endl;
            }
        }

        serviceSock.close();
    }

    catch (OSG_STDEXCEPTION_NAMESPACE::exception &) { throw; }
}

void VRSceneSync::start() {
    if (_connection) delete _connection; // reset conneciton
    _connection = NULL;
    _aspect = new RemoteAspect(); // create aspect

    // accept incomming connections
    try {
        UInt8                    forceNetworkOrder;
#if BYTE_ORDER == LITTLE_ENDIAN
        UInt8                    littleEndian = true;
#else
        UInt8                    littleEndian = false;
#endif

        // accept
        acceptClient();

        // determine network order
        _connection->putValue(littleEndian);
        _connection->flush();
        _connection->selectChannel();
        _connection->getValue(forceNetworkOrder);
        _connection->setNetworkOrder((forceNetworkOrder != 0));
    } catch(...) { throw; }
}

void VRSceneSync::stop() {
    try { // destroy connection
        delete _connection;
        _connection = NULL;
    }
    catch (...) {}

    // destroy aspect
    delete _aspect;

    // reset
    _connection   = NULL;
    _aspect       = NULL;
}

void VRSceneSync::sync() { // corresponds to ClusterServer::doSync
    _aspect->receiveSync(*_connection, false); // sync with render client
    commitChangesAndClear();
}

void VRSceneSync::simpleUpdate() {
    try {
        sync();
        Thread::getCurrentChangeList()->clear();
    }

    catch(OSG_STDEXCEPTION_NAMESPACE::exception &e) {
        SLOG << e.what() << endLog;
        stop();
        start();
    }

    catch ( ... ) {
        SLOG << "ERROR, unknown error thrown in display()" << endLog;
        stop();
        start();
    }
}

void VRSceneSync::test() {
    VRSceneSync sync;
    sync.start();
}
