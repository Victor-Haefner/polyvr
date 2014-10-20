/*---------------------------------------------------------------------------*\
 *                                OpenSG                                     *
 *                                                                           *
 *                                                                           *
 *             Copyright (C) 2000-2002 by the OpenSG Forum                   *
 *                                                                           *
 *                            www.opensg.org                                 *
 *                                                                           *
 *   contact: dirk@opensg.org, gerrit.voss@vossg.org, jbehr@zgdv.de          *
 *                                                                           *
\*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*\
 *                                License                                    *
 *                                                                           *
 * This library is free software; you can redistribute it and/or modify it   *
 * under the terms of the GNU Library General Public License as published    *
 * by the Free Software Foundation, version 2.                               *
 *                                                                           *
 * This library is distributed in the hope that it will be useful, but       *
 * WITHOUT ANY WARRANTY; without even the implied warranty of                *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Library General Public License for more details.                          *
 *                                                                           *
 * You should have received a copy of the GNU Library General Public         *
 * License along with this library; if not, write to the Free Software       *
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                 *
 *                                                                           *
\*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*\
 *                                Changes                                    *
 *                                                                           *
 *                                                                           *
 *                                                                           *
 *                                                                           *
 *                                                                           *
 *                                                                           *
\*---------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <boost/bind.hpp>

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGThread.h>
#include <OpenSG/OSGThreadManager.h>
#include "OSGClusterServer.h"
#include <OpenSG/OSGPointConnection.h>
#include <OpenSG/OSGConnectionFactory.h>
#include <OpenSG/OSGDgramSocket.h>
#include <OpenSG/OSGClusterWindow.h>
//#include "OSGClusterWindow.h"
#include <OpenSG/OSGBinaryMessage.h>
#include <OpenSG/OSGClusterNetwork.h>
#include <OpenSG/OSGRemoteAspect.h>
#include <OpenSG/OSGTypeFactory.h>

#include <OpenSG/OSGImageComposer.h>

// testing
//#include "OSGMultiDisplayWindow.h"

#include <OpenSG/OSGOSGWriter.h>

OSG_USING_NAMESPACE

/*! \class OSG::ClusterServer
    \brief Cluster rendering server

   A ClusterServer is responsible for syncronizing all client changes.
   Each cluster renderer can offer it's service by a symbolic name.
   So it is possible to have a server called "left" or "right".
   The server uses a local Qt or GLUT window for rendering.
   <pre>
   // create a server
   GLUTWindowPtr window=GLUTWindow::create();
   server = new ClusterServer(window,"server1","Multicast");
   // wait for clients to connect
   server->init();
   ...
   // render
   server->render(ract);
   </pre>
 */

std::vector<InitFuncF> *ClusterServer::osgInitFunctions = NULL;
std::vector<ExitFuncF> *ClusterServer::osgExitFunctions = NULL;

void ClusterServer::addInitFunction(InitFuncF initFunc)
{
    if(osgInitFunctions == NULL)
    {
        osgInitFunctions = new std::vector<InitFuncF>(0);
    }

    osgInitFunctions->push_back(initFunc);
}

void ClusterServer::addExitFunction(ExitFuncF exitFunc)
{
    if(osgExitFunctions == NULL)
    {
        osgExitFunctions = new std::vector<ExitFuncF>(0);
    }

    osgExitFunctions->push_back(exitFunc);
}

bool ClusterServer::init(Int32,
                         Char8 **)
{
    bool returnValue = true;

    if(GlobalSystemState != Running)
    {
        FFATAL(("ClusterServer::init: System not initialized; calls is "
                "NOT allowed.\n"));

        returnValue = false;
    }

    if(osgInitFunctions != NULL)
    {
        for(UInt32 i = 0; i < osgInitFunctions->size(); i++)
        {
            returnValue &= (*osgInitFunctions)[i]();

            if(returnValue == false)
                break;
        }

        osgInitFunctions->clear();
    }

    return returnValue;
}

bool ClusterServer::exit(void)
{
    bool returnValue = true;

    if(GlobalSystemState != Running)
    {
         return true;
    }

    if(osgExitFunctions != NULL)
    {
        for(PtrDiffT i = osgExitFunctions->size() - 1; i >= 0; i--)
        {
            returnValue &= (*osgExitFunctions)[i]();

            if(returnValue == false)
                break;
        }
    }

    delete osgExitFunctions;

    return returnValue;
}

/*-------------------------------------------------------------------------*/
/*                            Constructors                                 */

/*! Constructor
 *
 * \param window          rendering window. e.g. a Qt or GLUT window
 * \param serviceName     wait for connections that request this name
 * \param connectionType  network type. e.g. "Multicast"
 * \param address         address to wait for connections
 * \param servicePort     port to wait for connections
 * \param serviceGroup    service group
 *
 */
ClusterServer::ClusterServer(const std::string &serviceName,
                             const std::string &connectionType,
                             const std::string &address,
                                        UInt32  servicePort,
                             const std::string &serviceGroup):
    _connection(NULL),
    _requestAddress(address),
    _boundAddress(""),
    _clusterWindow(),
    _aspect(NULL),
    _serviceName(serviceName),
    _connectionType(connectionType),
    _servicePort(servicePort),
    _serviceGroup(serviceGroup),
    _serverId(0),
    _interface("")
{
    char localhost[256];

    // default is hostname
    if(_serviceName.empty())
    {
        osgGetHostname(localhost,255);
        _serviceName = localhost;
    }
    // if service contains ":" than treat as address
    if(_requestAddress.empty())
    {
        if(strstr(_serviceName.c_str(),":"))
            _requestAddress = _serviceName;
    }
}

/*-------------------------------------------------------------------------*/
/*                             Destructor                                  */

/*! Destructor. Disconnect from all connected rendering servers
 */

ClusterServer::~ClusterServer(void)
{
    _windows.clear();

    try
    {
        delete _connection;
        delete _aspect;
    }

    catch(...)
    {
    }
}

/*-------------------------------------------------------------------------*/
/*                             Class specific                              */


/*! start server
 *
 * Start cluster server and wait for a client to connect. This method
 * will return after a client connection or an error situation.
 */

void ClusterServer::start(void)
{
    OSG::FieldContainerType *fct;

    // reset conneciton

    delete _connection;

    _connection = NULL;

    // create aspect
    _aspect = new RemoteAspect();

    // register interrest for all changed cluster windows
    for(UInt32 i = 1;
               i < OSG::TypeFactory::the()->getNumTypes();
             ++i)
    {
        fct = OSG::FieldContainerFactory::the()->findType(i);

        if(fct && fct->isDerivedFrom(ClusterWindow::getClassType()))
        {
            _aspect->registerChanged(
                *fct,
                boost::bind(&ClusterServer::windowChanged, this, _1, _2));
        }
    }

    // accept incomming connections
    try
    {
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
    }
    catch(...)
    {
        throw;
    }
}

/*! Stop cluster server, remove current remote aspect and all its
    field containers.
 */

void ClusterServer::stop() {
    // get aspect ownership
    if(_clusterWindow != NULL) {
        _aspect = _clusterWindow->getNetwork()->getAspect();

        _clusterWindow->getNetwork()->setAspect(NULL);

        // That's the app one we will never receive as without the
        // app window it can not be send.
        _clusterWindow->subReferenceUnresolved();
        _clusterWindow = NULL;
    }

    for(_w_itr = _windows.begin(); _w_itr != _windows.end(); _w_itr++)
        (*_w_itr)->resolveLinks();

    // destroy connection

    try {
        delete _connection;
        _connection = NULL;
    }
    catch(...) { }

    // destroy aspect
    delete _aspect;

    // reset
    _connection   = NULL;
    _aspect       = NULL;
}

#ifdef OSG_OLD_RENDER_ACTION

/*! sync with client and render scenegraph
 */

void ClusterServer::render(DrawActionBase *action) {
    doSync  (false );
    doRender(action);
    doSwap  (      );
}

#endif

/*! sync with client and render scenegraph
 */

void ClusterServer::render(RenderActionBase *action) {
    doSync  (false );
    doRender(action);
    doSwap  (      );
}


/*! Synchronize all field containers with the client and call
 *  <code>serverInit</code>, <code>serverRender</code> and
 *  <code>serverSwap</code> for the cluster window.
 *  The cluster server uses the first synced ClusterWindow that
 *  contains the name of this server. <code>serverInit</code> is
 *  called after the first ClusterWindow sync.
 *
 *  todo: Sync RenderAciton contents
 */

void ClusterServer::doSync(bool applyToChangelist)
{
    // do we have a cluster window?
    if(_clusterWindow == NULL) {
        do _aspect->receiveSync(*_connection,applyToChangelist); // recive
        while(_clusterWindow == NULL);

        // get server id
        for(_serverId = 0;
             (_clusterWindow->getServers(_serverId) != _serviceName) &&
             (_serverId < _clusterWindow->getMFServers()->size());
            _serverId++) ;

        // server connected and cluster window found
        SINFO << "Start server " << _serviceName
              << " with id "     << _serverId
              << std::endl;

      // now the window is responsible for connection and aspect

        _clusterWindow->getNetwork()->setMainConnection(_connection);
        _clusterWindow->getNetwork()->setAspect        (_aspect);

        _connection = NULL;
        _aspect     = NULL;

        _clusterWindow->setDrawerId(_serverId);
        std::cout << "\nW size " << _windows.size() << std::endl;

        std::vector<Window*>::iterator itr;
        for (itr = _windows.begin(); itr != _windows.end(); itr++)
            _clusterWindow->serverInit(*itr, _serverId);
    }

    RemoteAspect *aspect     = _clusterWindow->getNetwork()->getAspect();
    Connection   *connection =
        _clusterWindow->getNetwork()->getMainConnection();

    // sync with render clinet
    aspect->receiveSync(*connection, applyToChangelist);

    // sync with render client
    if(_clusterWindow->getInterleave())
    {
        // if the reminder of the division of interleave and
        // framecount is equal to the servers id, the right
        // sync point for the current render frame is reached
        while( ( _clusterWindow->getFrameCount() %
                 _clusterWindow->getInterleave() )  !=
               (_serverId%_clusterWindow->getInterleave()) )
        {
            aspect->receiveSync(*connection, applyToChangelist);
        }
    }

    if(applyToChangelist) commitChanges();
    else commitChangesAndClear();
}

/*! render server window
 */

void ClusterServer::doRender(RenderActionBase *action)
{
#if 0
    OSG::IndentFileOutStream outFileStream("/tmp/cluster.osg");

    if(outFileStream)
    {
        //std::cerr << "STARTING PRINTOUT:" << std::endl;

        OSG::OSGWriter writer(outFileStream, 4);

        writer.write(_clusterWindow);

        outFileStream.close();
    }
#endif

    std::vector<Window*>::iterator itr;
    for (itr = _windows.begin(); itr != _windows.end(); itr++)
        _clusterWindow->serverRender(*itr, _serverId, action);
}

/*! swap server window
 */

void ClusterServer::doSwap(void) {
    std::vector<Window*>::iterator itr;
    for (itr = _windows.begin(); itr != _windows.end(); itr++)
        _clusterWindow->serverSwap(*itr, _serverId);
}

/*! return the cluster window received from the client
 */

Window *ClusterServer::getClusterWindow(int i)
{
    return _clusterWindow;
}

/*! return the window used for rendering
 */

Window *ClusterServer::getServerWindow(int i) {
    if (_windows.size() > (uint)i) return _windows[i];
    return 0;
}

/*! clusterWindow changed callback. This is a callback functor.
    It is called for each change of a ClusterWindow.
 */

bool ClusterServer::windowChanged(FieldContainer * const fcp,
                                  RemoteAspect   *          )
{
    if(_clusterWindow != NULL)
        return true;

    ClusterWindow *window = dynamic_cast<ClusterWindow *>(fcp);

    if(window->getMFServers()->size())
    {
        MFString::const_iterator sIt =
            window->getMFServers()->find(_serviceName);

        if(sIt == window->getMFServers()->end())
        {
            SWARNING << "wrong window" << std::endl;
        }
        else
        {
            _clusterWindow = window;

            for(_w_itr = _windows.begin(); _w_itr != _windows.end(); _w_itr++) {
                WindowUnrecPtr _window = *_w_itr;

                fprintf(stderr, "%p %" PRISize " %td\n",
                        &(*_window),
                        window->getMFServers()->size(),
                        sIt - window->getMFServers()->begin());

                const MFUInt32 &vIds = *(window->getMFServerIds());

                if(window->getMFServers()->size() <= vIds.size())
                {
                    _window->setDrawerId(vIds[sIt -
                                              window->getMFServers()->begin()]);
                }
                else
                {
                    _window->setDrawerId(sIt - window->getMFServers()->begin());
                }
            }
        }
    }

    return true;
}

/*! Wait for incomming clients. A client can send a request for a
 *  special connection type or it can try to connect it it knows
 *  the servers address.
 */

void ClusterServer::acceptClient(void)
{
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

    catch(OSG_STDEXCEPTION_NAMESPACE::exception &)
    {
        throw;
    }
}

void ClusterServer::addWindow( Window* window ) {
    _windows.push_back(window);
}
