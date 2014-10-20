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

#ifndef _OSGCLUSTERWINDOW_H_
#define _OSGCLUSTERWINDOW_H_
#ifdef __sgi
#pragma once
#endif

#include <OpenSG/OSGClusterWindowBase.h>
#include <OpenSG/OSGStatElemTypes.h>

#include <boost/function.hpp>

OSG_BEGIN_NAMESPACE

class StatCollector;
class Connection;
class ClusterServer;
class RemoteAspect;
class ClusterNetwork;
OSG_GEN_MEMOBJPTR(ClusterNetwork);
class RenderActionBase;

/*! \ingroup GrpClusterWindowObj
    \ingroup GrpLibOSGCluster
    \includebasedoc
 */

class OSG_CLUSTER_DLLMAPPING ClusterWindow : public ClusterWindowBase
{
  private:

    /*==========================  PUBLIC  =================================*/

  public:

    typedef ClusterWindowBase Inherited;

    /*---------------------------------------------------------------------*/
    /*! \name                   window functions                           */
    /*! \{                                                                 */

    virtual void changed(ConstFieldMaskArg whichField,
                         UInt32            origin,
                         BitVector         detail);

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                     Output                                   */
    /*! \{                                                                 */

    virtual void dump(      UInt32     uiIndent = 0,
                      const BitVector  bvFlags  = 0) const;

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name            GL implementation functions                       */
    /*! \{                                                                 */

#if 0
    virtual void(*getFunctionByName (const Char8 *s))();
#endif

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name      Window system implementation functions                  */
    /*! \{                                                                 */

    virtual void  init(GLInitFunctor oFunc = GLInitFunctor());

    virtual void  render            (RenderActionBase *action);

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name      Window system implementation functions                  */
    /*! \{                                                                 */

    virtual void activate  (void);
    virtual void deactivate(void);
    virtual bool swap      (void);

    virtual void terminate (void);

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name            asynchronous initialization                       */
    /*! \{                                                                 */

    typedef boost::function<bool (const std::string &msg,
                                  const std::string &server,
                                        Real32       progress)> ConnectionCB;

    bool initAsync      (const ConnectionCB &fp);
    void setConnectionCB(const ConnectionCB &fp);

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name               connection pool                                */
    /*! \{                                                                 */

    ClusterNetwork *getNetwork(void);

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                   Statistics                                 */
    /*! \{                                                                 */

    StatCollector *getStatistics(void               ) const;
    void           setStatistics(StatCollector *stat);

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                  static stat elem                            */
    /*! \{                                                                 */

    static StatElemDesc<StatTimeElem> statActivateTime;
    static StatElemDesc<StatTimeElem> statFrameInitTime;
    static StatElemDesc<StatTimeElem> statRAVTime;
    static StatElemDesc<StatTimeElem> statSwapTime;
    static StatElemDesc<StatTimeElem> statFrameExitTime;

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                   Calibration                                */
    /*! \{                                                                 */

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                   Exceptions                                 */
    /*! \{                                                                 */

    /*! \nohierarchy
     */

    class OSG_CLUSTER_DLLMAPPING AsyncCancel : public Exception
    {
      public:
        AsyncCancel();
    };

    /*! \}                                                                 */
    /*=========================  PROTECTED  ===============================*/

  protected:

    /*---------------------------------------------------------------------*/
    /*! \name      client window funcitons                                 */
    /*! \{                                                                 */

    virtual void clientInit   (void                    );
    virtual void clientPreSync(void                    );

    virtual void clientRender (RenderActionBase *action);
    virtual void clientSwap   (void                    );

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name      server window funcitons                                 */
    /*! \{                                                                 */

    virtual void serverInit  (std::vector<Window*> windows,
                              UInt32          id    );
    virtual void serverRender(std::vector<Window*> windows,
                              UInt32            id,
                              RenderActionBase *action);
    virtual void serverSwap  (std::vector<Window*> windows,
                              UInt32            id    );

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                  Constructors / Destructor                   */
    /*! \{                                                                 */

    ClusterWindow(void);
    ClusterWindow(const ClusterWindow &source);

    virtual ~ClusterWindow(void);

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name               unsynced thread variables                      */
    /*! \{                                                                 */

    bool               _firstFrame;
    StatCollector     *_statistics;

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name      Window system implementation functions                  */
    /*! \{                                                                 */

    virtual void  doFrameInit         (bool reinitExtFuctions = false);
    virtual void  doFrameExit         (void                          );
    virtual void  doActivate          (void                          );
    virtual void  doDeactivate        (void                          );
    virtual bool  doSwap              (void                          );

    virtual void  doRenderAllViewports(RenderActionBase *action      );

    virtual bool  hasContext          (void                          );

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name              init method                                     */
    /*! \{                                                                 */

    static void initMethod(InitPhase ePhase);
    static void exitMethod(InitPhase ePhase);

    /*! \}                                                                 */
    /*==========================  PRIVATE  ================================*/

  private:

    /*---------------------------------------------------------------------*/
    /*! \name               private members                                */
    /*! \{                                                                 */

    ConnectionCB         _connectionFP;
    ClusterNetworkRefPtr _network;

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/

    friend class FieldContainer;
    friend class ClusterWindowBase;
    friend class ClusterServer;
    friend class ClusterClient;

    // prohibit default functions (move to 'public' if you need one)
    void operator =(const ClusterWindow &source);
};

typedef ClusterWindow *ClusterWindowP;

OSG_END_NAMESPACE

#include "OSGClusterWindow.inl"
#include "OSGClusterWindowBase.inl"

#endif /* _OSGCLUSTERWINDOW_H_ */
