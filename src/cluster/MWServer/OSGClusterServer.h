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

#ifndef _CLUSTERSERVER_H_
#define _CLUSTERSERVER_H_
#ifdef __sgi
#pragma once
#endif

#include <OpenSG/OSGClusterDef.h>
//#include "OSGClusterWindow.h"
#include <OpenSG/OSGClusterWindow.h>

OSG_BEGIN_NAMESPACE

class PointConnection;
class ClusterWindow;
class RemoteAspect;

/*! \ingroup GrpClusterBase
    \ingroup GrpLibOSGCluster
 */

class OSG_CLUSTER_DLLMAPPING ClusterServer
{
    /*==========================  PUBLIC  =================================*/

  public:

    /*---------------------------------------------------------------------*/
    /*! \name                   Constructors                               */
    /*! \{                                                                 */

    ClusterServer(const std::string    &serviceName,
                  const std::string    &connectionType = "StreamSock",
                  const std::string    &address        = "",
                             UInt32     servicePort    = 8437,
                  const std::string    &serviceGroup   = "");

    void addWindow( Window* window );

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                   Destructor                                 */
    /*! \{                                                                 */

    virtual ~ClusterServer(void);

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                   server actions                             */
    /*! \{                                                                 */

    void start   (void                               );
    void stop    (void                               );
#ifdef OSG_OLD_RENDER_ACTION
    void render  (DrawActionBase *action             );
#endif

#ifdef OSG_OLD_RENDER_ACTION
    void doRender(DrawActionBase *action             );
#endif

    void render  (RenderActionBase *action           );
    void doSync  (bool              applyToChangelist);
    void doRender(RenderActionBase *action           );

    void doSwap  (void                               );

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                   window access                              */
    /*! \{                                                                 */

    Window       *getClusterWindow(int i);
    Window       *getServerWindow (int i);

    RemoteAspect *getRemoteAspect (void) const;
    UInt32        getServerId     (void) const;

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                   set                                        */
    /*! \{                                                                 */

    void setInterface(const std::string &interf);

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name               cluster server app init                        */
    /*! \{                                                                 */

    static void addInitFunction      (InitFuncF initFunc);
    static void addExitFunction      (ExitFuncF exitFunc);
    static bool init                 (Int32    argc,
                                      Char8  **argv     );
    static bool exit                 (void              );

    /*! \}                                                                 */
    /*=========================  PROTECTED  ===============================*/

  protected:

    static std::vector<InitFuncF> *osgInitFunctions;
    static std::vector<ExitFuncF> *osgExitFunctions;

    /*---------------------------------------------------------------------*/
    /*! \name            ClusterWindow changed function                    */
    /*! \{                                                                 */

    bool windowChanged(FieldContainer * const fcp,
                       RemoteAspect   *          );

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                      Member                                  */
    /*! \{                                                                 */

    std::vector<Window*> _windows;
    std::vector<Window*>::iterator _w_itr;
    PointConnection       *_connection;
    std::string            _requestAddress;
    std::string            _boundAddress;
    ClusterWindowUnrecPtr  _clusterWindow;
    RemoteAspect          *_aspect;
    std::string            _serviceName;
    std::string            _connectionType;
    UInt32                 _servicePort;
    std::string            _serviceGroup;
    UInt32                 _serverId;
    std::string            _interface;

    /*! \}                                                                 */
    /*==========================  PRIVATE  ================================*/

  private:

    /*---------------------------------------------------------------------*/
    /*! \name                     helper function                          */
    /*! \{                                                                 */

    void acceptClient();

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/

    /*!\brief prohibit default function (move to 'public' if needed) */
    ClusterServer(const ClusterServer &source);
    /*!\brief prohibit default function (move to 'public' if needed) */
    void operator =(const ClusterServer &source);
};

OSG_END_NAMESPACE

#include "OSGClusterServer.inl"

#endif /* _CLUSTERSERVER_H_ */

