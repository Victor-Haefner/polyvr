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

//---------------------------------------------------------------------------
//  Includes
//---------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>

#include <sstream>

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGSystemDef.h>
#include "OSGClusterWindow.h"
#include <OpenSG/OSGDgramSocket.h>
#include <OpenSG/OSGStreamSocket.h>
#include <OpenSG/OSGRemoteAspect.h>
#include <OpenSG/OSGConnection.h>
#include <OpenSG/OSGBinaryMessage.h>
#include <OpenSG/OSGRemoteAspect.h>
#include <OpenSG/OSGConnectionFactory.h>
#include <OpenSG/OSGClusterNetwork.h>
#include <OpenSG/OSGGroupSockConnection.h>
#include <OpenSG/OSGStatCollector.h>

#if 0
#include <OpenSG/OSGDisplayCalibration.h>
#include <OpenSG/OSGImageComposer.h>
#endif

OSG_USING_NAMESPACE

/** \class OSG::ClusterWindow
 *  \ingroup GrpSystemCluster
 *  \brief Abstract base class for cluster configurations
 *
 * A ClusterWindow describes a clustering algorithm. A ClusterWindow
 * inherits the ability to connect rendering servers and initiate
 * remote rendering. By configuring the cluster algorithm with an
 * OpenSG Window structure, it is possible to use cluster rendering
 * in the same way as rendering in a GLUT or Qt window.
 *
 **/

/*StatElemDesc<StatTimeElem> ClusterWindow::statActivateTime
  ("statActivateTime", "time to activate remote window");

StatElemDesc<StatTimeElem> ClusterWindow::statFrameInitTime
  ("statFrameInitTime", "time to frameInit remote window");

StatElemDesc<StatTimeElem> ClusterWindow::statRAVTime
  ("statRAVTime", "time to RAV remote window");

StatElemDesc<StatTimeElem> ClusterWindow::statSwapTime
  ("statSwapTime", "time to swap remote window");

StatElemDesc<StatTimeElem> ClusterWindow::statFrameExitTime
  ("statFrameExitTime", "time to frameExit remote window");*/

/*-------------------------------------------------------------------------*/
/*                          window functions                               */

//! react to field changes

void ClusterWindow::changed(ConstFieldMaskArg whichField,
                            UInt32            origin,
                            BitVector         details)
{
    Inherited::changed(whichField, origin, details);
}

//! output the instance for debug purposes

void ClusterWindow::dump(      UInt32    ,
                         const BitVector ) const
{
    SLOG << "Dump ClusterWindow NI" << std::endl;
}

#if 0
void (*ClusterWindow::getFunctionByName(const Char8 *s))()
{
    return NULL;
}
#endif

/*! init cluster window. connect to all servers
 */

void ClusterWindow::init(GLInitFunctor)
{
    GroupConnection    *connection;
    RemoteAspect       *remoteAspect;
    UInt32             c, id;
    MFString::const_iterator  s;
    Connection::Channel channel;
    bool                directConnect=false;

    if(getNetwork()->getMainConnection())
    {
        SWARNING << "init called twice" << std::endl;
        return;
    }

    // create connection
    if(getConnectionType().empty())
    {
        setConnectionType("StreamSock");
    }

    connection = ConnectionFactory::the()->createGroup(getConnectionType());

    if(connection == NULL)
    {
        SFATAL << "Unknown connection type "
               << getConnectionType()
               << std::endl;
        return;
    }

    connection->setDestination(getConnectionDestination());
    connection->setInterface  (getConnectionInterface  ());
    connection->setParams     (getConnectionParams     ());

    getNetwork()->setMainConnection(connection);

    // create remote aspect

    remoteAspect = new RemoteAspect();

    getNetwork()->setAspect(remoteAspect);

    if(_statistics)
        remoteAspect->setStatistics(_statistics);

    // autostart servers
    std::string server;
    std::string autostart;
    std::string env;

    Real32 progress = 0.0f;
    Real32 progressStep = 1.0f / Real32(getMFServers()->size());

    if(getMFAutostart()->size())
    {
        progressStep /= 2;
        std::vector<FILE*>           pipes;

        for(id=0 ; id<getMFServers()->size() ; ++id)
        {
            std::ostringstream command;

            server    = (*getMFServers())[id];
            SizeT pos = server.find(":");

            if(pos != std::string::npos)
                server.erase(pos);

            autostart = (*getMFAutostart())[id % getMFAutostart()->size()];

            for(c = 0 ; c < autostart.length() ; ++c)
            {
                if(autostart[c] == '%' && c+1 < autostart.length())
                    switch(autostart[++c])
                    {
                        case 's':
                            command << server;
                            break;
                        case 'n':
                            command << (*getMFServers())[id];
                            break;
                        case 'i':
                            command << id;
                            break;
                        case '{':
                            env = "" ;
                            while(++c < autostart.length() &&
                                  autostart[c] != '}')
                                env += autostart[c];
                            if(getenv(env.c_str()))
                                command << getenv(env.c_str());
                            break;
                        case '%':
                            command << '%';
                            break;
                        default:
                            command << '%' << autostart[c];
                    }
                else
                    command << autostart[c];
            }
            SINFO << command.str() << std::endl;
#ifdef WIN32
            FILE *pipe = _popen(command.str().c_str(),"r");
#else
            FILE *pipe = popen(command.str().c_str(),"r");
#endif
            if(!pipe)
                SFATAL << "Error starting: " << command << std::endl;
            pipes.push_back(pipe);
        }

        for(id = 0 ; id < getMFServers()->size() ; ++id)
        {
            if(pipes[id])
            {
                // update progress
                if(_connectionFP != NULL)
                {
                    if( !_connectionFP("Starting:", (*getMFServers())[id], progress) ) {
                        // abort, cleanup remaining pipes
                        for( ; id<getMFServers()->size() ; ++id) {
                            if(pipes[id]) {
#ifdef WIN32
                                _pclose(pipes[id]);
#else
                                pclose(pipes[id]);
#endif
                            }
                            throw AsyncCancel();
                        }
                    }
                }
                SINFO << "Waiting for "
                      << getServers(id)
                      << " to start."
                      << std::endl;

                int result;
                std::string line="";

                while((result=fgetc(pipes[id])) != EOF)
                {
                    line += result;

                    if(result == '\n')
                    {
                        SINFO << line;
                        line = "";
                    }
                }

                if(!line.empty())
                    SINFO << line << std::endl;
#ifdef WIN32
                _pclose(pipes[id]);
#else
                pclose(pipes[id]);
#endif
                SINFO << getServers(id) << " started." << std::endl;

                progress += progressStep;
            }
        }
    }

    // connect to all servers
    for(s = getMFServers()->begin();
        s!= getMFServers()->end();
        s++)
    {
        DgramSocket      serviceSock;
        BinaryMessage    msg;
        std::string      respServer;
        std::string      respAddress;
        bool             retry=true;

        if(strstr((*s).c_str(),":"))
            directConnect = true;
        else
            directConnect = false;

        SINFO << "Connect to " << (*s) << std::endl;

        serviceSock.open();
        serviceSock.setTTL(8);

        // set interface
        if(!getServiceInterface().empty())
        {
            serviceSock.setMCastInterface(
                SocketAddress(getServiceInterface().c_str()));
        }

        while(retry)
        {
            try
            {
                // update progress
                if(_connectionFP != NULL)
                {
                    if(!_connectionFP("Connecting:", *s, progress))
                    {
                        serviceSock.close();
                        throw AsyncCancel();
                    }
                }

                // try to connect with the servers name
                try
                {
                    if(directConnect)
                    {
                        channel = connection->connectPoint(*s,0.5);
                        if(channel >= 0)
                        {
                            retry = false;

                            SINFO << "Connected with address:"
                                  << *s
                                  << std::endl;
                            break;
                        }
                    }
                }
                catch(...)
                {
                }

                // find server
                msg.clear();
                msg.putString(*s);
                msg.putString(getConnectionType());

                if(_sfServiceAddress.getValue().size() != 0)
                {
                    SINFO << "send request to:"
                          << _sfServiceAddress.getValue()
                          << std::endl;

                    try
                    {
                        serviceSock.sendTo(
                            msg,SocketAddress(
                                _sfServiceAddress.getValue().c_str(),
                                getServicePort()));
                    }
                    catch(AsyncCancel &)
                    {
                        throw;
                    }
                    catch(OSG_STDEXCEPTION_NAMESPACE::exception &e)
                    {
                        SINFO << e.what() << std::endl;
                    }
                }
                SINFO << "send request to:"
                      << SocketAddress(SocketAddress::BROADCAST,
                                       getServicePort()).getHost().c_str()
                      << std::endl;

                try
                {
                    serviceSock.sendTo(
                        msg,SocketAddress(SocketAddress::BROADCAST,
                                          getServicePort()));
                }
                catch(AsyncCancel &)
                {
                    throw;
                }
                catch(OSG_STDEXCEPTION_NAMESPACE::exception &e)
                {
                    SINFO << e.what() << std::endl;
                }

                if(serviceSock.waitReadable(0.1))
                {
                    SocketAddress from;
                    serviceSock.recvFrom(msg, from);
                    msg.getString(respServer);
                    msg.getString(respAddress);

                    if(respServer == *s)
                    {
                        GroupSockConnection *pointSock =
                            dynamic_cast<GroupSockConnection*> (connection);

                        if(pointSock != NULL)
                        {
                            /* for all socket connections ignore the
                               incoming host and use the host from
                               the last response. */

                            char port[16];

                            if(sscanf(respAddress.c_str(),
                                      "%*[^:]:%15s",port) == 1)
                            {
                                respAddress = from.getHost() + ":" + port;
                            }
                        }

                        SINFO << "Found at address "
                              << respAddress
                              << std::endl;

                        // connect to server
                        channel = connection->connectPoint(respAddress);

                        if(channel >= 0)
                            retry=false;
                    }
                }
            }
            catch(AsyncCancel &)
            {
                throw;
            }
            catch(OSG_STDEXCEPTION_NAMESPACE::exception &e)
            {
                SINFO << e.what() << std::endl;
            }
        }

        serviceSock.close();

        progress += progressStep;
    }

    // determine byte order
    UInt8 serverLittleEndian;
    UInt8 forceNetworkOrder=false;
#if BYTE_ORDER == LITTLE_ENDIAN
    UInt8 littleEndian = true;
#else
    UInt8 littleEndian = false;
#endif

    for(UInt32 i=0;i<getMFServers()->size();++i)
    {
        channel = connection->selectChannel();
        connection->subSelection(channel);
        connection->getValue(serverLittleEndian);

        if(serverLittleEndian != littleEndian)
        {
            forceNetworkOrder=true;
        }
    }
    connection->resetSelection();
    // tell the servers the encoding mode
    connection->putValue(forceNetworkOrder);
    connection->flush();
    connection->setNetworkOrder((forceNetworkOrder != 0));

    if(forceNetworkOrder)
    {
        SINFO << "Run clustering in network order mode" << std::endl;
    }

    // inform connection finished
    if(_connectionFP != NULL)
        _connectionFP("ClusterWindow initialization completed.", "", 1.0);
}

bool ClusterWindow::initAsync(const ConnectionCB &fp)
{
    bool result;
    ConnectionCB saveFP = _connectionFP;

    _connectionFP = fp;

    try
    {
        init();
        result = true;
    }
    catch(AsyncCancel &)
    {
        result = false;
    }
    _connectionFP = saveFP;

    return result;
}

void ClusterWindow::setConnectionCB(const ConnectionCB &fp)
{
    _connectionFP = fp;
}

#ifdef OSG_OLD_RENDER_ACTION
void ClusterWindow::render(DrawActionBase *action)
{
    activate();
    frameInit();
    renderAllViewports(action);
    swap();
    frameExit();
}
#endif

void ClusterWindow::render(RenderActionBase *action)
{
    if(_statistics != NULL)
        _statistics->getElem(statActivateTime)->start();

    doActivate();

    if(_statistics != NULL)
        _statistics->getElem(statActivateTime)->stop();


    if(_statistics != NULL)
        _statistics->getElem(statFrameInitTime)->start();

    doFrameInit();

    if(_statistics != NULL)
        _statistics->getElem(statFrameInitTime)->stop();


    if(_statistics != NULL)
        _statistics->getElem(statRAVTime)->start();

    doRenderAllViewports(action);

    if(_statistics != NULL)
        _statistics->getElem(statRAVTime)->stop();


    if(_statistics != NULL)
        _statistics->getElem(statSwapTime)->start();

    doSwap();

    if(_statistics != NULL)
        _statistics->getElem(statSwapTime)->stop();


    if(_statistics != NULL)
        _statistics->getElem(statFrameExitTime)->start();

    doFrameExit();

    if(_statistics != NULL)
        _statistics->getElem(statFrameExitTime)->stop();
}

void ClusterWindow::activate(void)
{
    this->doActivate();
}

void ClusterWindow::deactivate(void)
{
    this->doDeactivate();
}

bool ClusterWindow::swap(void)
{
    return this->doSwap();
}

void ClusterWindow::terminate (void)
{
}

void ClusterWindow::doActivate(void)
{
}

void ClusterWindow::doDeactivate(void)
{
}

bool ClusterWindow::doSwap(void)
{
    if(getNetwork()->getMainConnection() && getNetwork()->getAspect())
    {
        clientSwap();
    }

    return true;
}

#ifdef OSG_OLD_RENDER_ACTION
void ClusterWindow::renderAllViewports(DrawActionBase *action)
{
    if(getNetwork()->getMainConnection() && getNetwork()->getAspect())
    {
        clientRender(action);
    }
}
#endif

void ClusterWindow::doRenderAllViewports(RenderActionBase *action)
{
    if(getNetwork()->getMainConnection() && getNetwork()->getAspect())
    {
        clientRender(action);
    }
}

bool ClusterWindow::hasContext(void)
{
    return true;
}

void ClusterWindow::doFrameInit(bool reinitExtFuctions)
{
    Connection   *connection   = getNetwork()->getMainConnection();
    RemoteAspect *remoteAspect = getNetwork()->getAspect();

    if(remoteAspect && connection)
    {
        if(_firstFrame)
        {
            setFrameCount(0);

            // send sync

            commitChanges();
            remoteAspect->sendSync(*connection);

            ChangeList *cl = ChangeList::create();

            cl->clear();
            cl->merge(*Thread::getCurrentChangeList());

            Thread::getCurrentChangeList()->clear();

            // init client window
            clientInit();
            // last chance to modifie before sync
            clientPreSync();
            // send sync
            commitChanges();
            remoteAspect->sendSync(*connection);

//            cl.merge(*Thread::getCurrentChangeList());
//            Thread::getCurrentChangeList()->clear();
            Thread::getCurrentChangeList()->merge(*cl);

            OSG::subRef(cl);

            _firstFrame = false;
        }
        else
        {
            setFrameCount(getFrameCount() + 1);
            clientPreSync();

            commitChanges();
            remoteAspect->sendSync(*connection);
        }
    }
}

void ClusterWindow::doFrameExit(void)
{
}

/*-------------------------------------------------------------------------*/
/*                          statistics                                     */

void ClusterWindow::setStatistics(StatCollector *statistics)
{
    _statistics = statistics;
    if(getNetwork()->getAspect())
        getNetwork()->getAspect()->setStatistics(statistics);
}

/*-------------------------------------------------------------------------*/
/*                          calibration                                    */

/*-------------------------------------------------------------------------*/
/*                          exceptions                                     */

ClusterWindow::AsyncCancel::AsyncCancel()
{
}

/*-------------------------------------------------------------------------*/
/*                         client methods                                  */

/*! init client window. In a derived cluster window this method is called
 *  before the first sync with the rendering servers. There is no default
 *  action.
 */
void ClusterWindow::clientInit(void)
{
}

/** client frame before sync
 *
 * In a derived cluster window this method is called before
 * sync with the rendering servers. Default aciton is to activate
 * and init the client window.
 **/

void ClusterWindow::clientPreSync( void )
{
    if(getClientWindow() != NULL)
    {
        getClientWindow()->activate ();
        getClientWindow()->frameInit();
    }
}

/** initiate client rendering
 *
 * In a derived cluster window this method is called after the
 * sync with all rendering servers. Default aciton is to render all
 * viewports of the client window.
 **/

void ClusterWindow::clientRender(RenderActionBase *action)
{
    if(getClientWindow() != NULL)
    {
        getClientWindow()->renderAllViewports(action);
    }
}

/** swap client window
 *
 * In a derived cluster window this method is called after rendering
 * Default aciton is to swap the local client window.
 **/

void ClusterWindow::clientSwap( void )
{
    if(getClientWindow() != NULL)
    {
        getClientWindow()->swap     ();
        getClientWindow()->frameExit();
    }

#if 0
    if(getDirty() == true)
    {
        setDirty(false);
    }
#endif
}

/*-------------------------------------------------------------------------*/
/*                         server methods                                  */



/** initialise the cluster window on the server side
 *
 * This method is called after the first sync.
 *
 * \param window     server render window
 * \param id         server id
 **/

void ClusterWindow::serverInit(std::vector<Window*> windows,
                               UInt32  )
{
}

/** render server window
 *
 * This method is called after synchronisation of all changes with the
 * rendering client. Default action is to render all viewports with the
 * given action
 *
 * !param window     server render window
 * !param id         server id
 * !param action     action
 **/

#ifdef OSG_OLD_RENDER_ACTION
void ClusterWindow::serverRender(std::vector<Window*> windows,
                                 UInt32          id,
                                 DrawActionBase *action )
{
#if 0
    window->activate();
    window->frameInit();
    window->renderAllViewports(action);
#endif

    window->renderNoFinish(action);

#if 0
    RenderOptionsPtr ro;

    RenderAction *ract = dynamic_cast<RenderAction *>(action);
    if(ract != NULL)
    {
        MFViewportPtr::iterator       portIt  = window->getPort().begin();
        MFViewportPtr::const_iterator portEnd = window->getPort().end();
        // try to find option as an attachment of window
        OSG::RenderOptionsPtr winRo = OSG::RenderOptionsPtr::dcast(
            window->findAttachment(OSG::RenderOptions::getClassType()));
        ract->setWindow(window.getCPtr());
        while(portIt != portEnd)
        {
            // try to find option an attachment at the viewport
            OSG::RenderOptionsPtr vpRo = OSG::RenderOptionsPtr::dcast(
                (*portIt)->findAttachment(OSG::RenderOptions::getClassType()));
            // try to find option an attachment at the root node
            OSG::RenderOptionsPtr rootRo = NULL;
            if((*portIt)->getRoot() != NULL)
            {
                rootRo = OSG::RenderOptionsPtr::dcast(
                    (*portIt)->getRoot()->findAttachment(OSG::RenderOptions::getClassType()));
            }
            if(rootRo != NULL)
                ro = rootRo;
            else
                if(vpRo != NULL)
                    ro = vpRo;
                else
                    ro = winRo;
            if(ro != NULL)
                ro->activateOptions(ract);
            (*portIt)->render(ract);
            ++portIt;
        }
    } else {
        if(action)
            window->renderAllViewports(action);
    }
#endif
}
#endif

void ClusterWindow::serverRender(std::vector<Window*> windows,
                                 UInt32            id,
                                 RenderActionBase *action )
{
    std::vector<Window*>::iterator itr;
    for (itr = windows.begin(); itr != windows.end(); itr++) {
        WindowUnrecPtr window = *itr;
        window->activate();
        window->frameInit();
        window->renderAllViewports(action);
    }
}

/** swap server window
 *
 * <code>serverSwap</code> is called after rendering. Default action is
 * to swap the rendering window.
 *
 * !param window     server render window
 * !param id         server id
 * !param connection connection to client
 **/
void ClusterWindow::serverSwap(std::vector<Window*> windows,
                               UInt32        )
{
    std::vector<Window*>::iterator itr;
    for (itr = windows.begin(); itr != windows.end(); itr++) {
        WindowUnrecPtr window = *itr;
        window->swap     ();
        window->frameExit();
    }
}

/*-------------------------------------------------------------------------*/
/*                         constructor / destructor                        */

//! Constructor

ClusterWindow::ClusterWindow(void) :
     Inherited(),
    _firstFrame  (true),
    _statistics  (NULL),
    _connectionFP(NULL),
    _network     (NULL)
{
}

//! Copy Constructor

ClusterWindow::ClusterWindow(const ClusterWindow &source) :
     Inherited   (source              ),
    _firstFrame  (true                ),
    _statistics  (NULL                ),
    _connectionFP(source._connectionFP),
    _network     (NULL                )
{
}

//! Destructor

ClusterWindow::~ClusterWindow(void)
{
    _network = NULL;
}

/*-------------------------------------------------------------------------*/
/*                           connection pool                               */

/*! Get connection pool
 */
ClusterNetwork *ClusterWindow::getNetwork(void)
{
    if(_network == NULL)
    {
        _network = ClusterNetwork::getInstance(this->getId());
    }

    return _network;
}

/*! initialize the static features of the class, e.g. action callbacks
 */

void ClusterWindow::initMethod(InitPhase ePhase)
{
    Inherited::initMethod(ePhase);
}

void ClusterWindow::exitMethod(InitPhase ePhase)
{
    Inherited::exitMethod(ePhase);
}
