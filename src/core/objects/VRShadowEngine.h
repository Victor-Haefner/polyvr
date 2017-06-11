#ifndef VRSHADOWENGINE_H_INCLUDED
#define VRSHADOWENGINE_H_INCLUDED

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

#include <OpenSG/OSGConfig.h>
#include "VRShadowEngineBase.h"

#include <OpenSG/OSGSimpleShadowMapEngineData.h>

#include <OpenSG/OSGDirectionalLight.h>
#include <OpenSG/OSGPointLight.h>

OSG_BEGIN_NAMESPACE

/*! \brief ShadowEngine is the basic NodeCore for inner nodes in the tree.
    \ingroup GrpGroupLightShadowEnginesObj
    \ingroup GrpLibOSGGroup
    \includebasedoc
*/

class OSG_GROUP_DLLMAPPING VRShadowEngine :
    public VRShadowEngineBase
{
    /*==========================  PUBLIC  =================================*/

  public:

    typedef SimpleShadowMapEngineData          EngineData;
    typedef SimpleShadowMapEngineData         *EngineDataPtr;
    typedef SimpleShadowMapEngineDataUnrecPtr  EngineDataUnrecPtr;

    /*---------------------------------------------------------------------*/
    /*! \name                       Sync                                   */
    /*! \{                                                                 */

    virtual void changed(ConstFieldMaskArg whichField,
                         UInt32            origin,
                         BitVector         detail);

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                       Render                                 */
    /*! \{                                                                 */

    virtual Action::ResultE runOnEnter(Light        *pLight,
                                       LightTypeE    eType,
                                       RenderAction *pAction);
    virtual Action::ResultE runOnLeave(Light        *pLight,
                                       LightTypeE    eType,
                                       RenderAction *pAction);

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                        Helper                                */
    /*! \{                                                                 */

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                        Dump                                  */
    /*! \{                                                                 */

    virtual void dump(      UInt32    uiIndent = 0,
                      const BitVector bvFlags  = 0) const;

    /*! \}                                                                 */
    /*=========================  PROTECTED  ===============================*/

  protected:

    typedef VRShadowEngineBase Inherited;

    /*---------------------------------------------------------------------*/
    /*! \name                   Constructors                               */
    /*! \{                                                                 */

    VRShadowEngine(void);
    VRShadowEngine(const VRShadowEngine &source);

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                   Destructors                                */
    /*! \{                                                                 */

    virtual ~VRShadowEngine(void);

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                        Init                                  */
    /*! \{                                                                 */

    static void initMethod(InitPhase ePhase);

    /*! \}                                                                 */
    /*---------------------------------------------------------------------*/
    /*! \name                       Action Callbacks                       */
    /*! \{                                                                 */

    void lightRenderEnter(Light         *pLight,
                          RenderAction  *pAction);

    void setupCamera     (Light         *pLight,
                          LightTypeE     eType,
                          RenderAction  *pAction,
                          EngineDataPtr  pEngineData);
    void setupLightChunk (Light         *pLight,
                          LightTypeE     eType,
                          RenderAction  *pAction,
                          EngineDataPtr  pEngineData);

    void doLightPass     (Light         *pLight,
                          RenderAction  *pAction,
                          EngineDataPtr  pEngineData);
    void doAmbientPass   (Light         *pLight,
                          RenderAction  *pAction,
                          EngineDataPtr  pEngineData);
    void doFinalPass     (Light         *pLight,
                          RenderAction  *pAction,
                          EngineDataPtr  pEngineData);

    /*! \}                                                                 */
    /*==========================  PRIVATE  ================================*/

  private:

    friend class FieldContainer;
    friend class VRShadowEngineBase;

    /*---------------------------------------------------------------------*/

    /*!\brief prohibit default function (move to 'public' if needed) */
    void operator =(const VRShadowEngine &source);
};

typedef VRShadowEngine              *VRShadowEngineP;






//! access the type of the class
inline
OSG::FieldContainerType &VRShadowEngineBase::getClassType(void)
{
    return _type;
}

//! access the numerical type of the class
inline
OSG::UInt32 VRShadowEngineBase::getClassTypeId(void)
{
    return _type.getId();
}

inline
OSG::UInt16 VRShadowEngineBase::getClassGroupId(void)
{
    return _type.getGroupId();
}

/*------------------------------ get -----------------------------------*/

//! Get the value of the VRShadowEngine::_sfShadowColor field.

inline
Color4f &VRShadowEngineBase::editShadowColor(void)
{
    editSField(ShadowColorFieldMask);

    return _sfShadowColor.getValue();
}

//! Get the value of the VRShadowEngine::_sfShadowColor field.
inline
const Color4f &VRShadowEngineBase::getShadowColor(void) const
{
    return _sfShadowColor.getValue();
}

//! Set the value of the VRShadowEngine::_sfShadowColor field.
inline
void VRShadowEngineBase::setShadowColor(const Color4f &value)
{
    editSField(ShadowColorFieldMask);

    _sfShadowColor.setValue(value);
}
//! Get the value of the VRShadowEngine::_sfForceTextureUnit field.

inline
Int32 &VRShadowEngineBase::editForceTextureUnit(void)
{
    editSField(ForceTextureUnitFieldMask);

    return _sfForceTextureUnit.getValue();
}

//! Get the value of the VRShadowEngine::_sfForceTextureUnit field.
inline
      Int32  VRShadowEngineBase::getForceTextureUnit(void) const
{
    return _sfForceTextureUnit.getValue();
}

//! Set the value of the VRShadowEngine::_sfForceTextureUnit field.
inline
void VRShadowEngineBase::setForceTextureUnit(const Int32 value)
{
    editSField(ForceTextureUnitFieldMask);

    _sfForceTextureUnit.setValue(value);
}


#ifdef OSG_MT_CPTR_ASPECT
inline
void VRShadowEngineBase::execSync (      VRShadowEngineBase *pFrom,
                                        ConstFieldMaskArg  whichField,
                                        AspectOffsetStore &oOffsets,
                                        ConstFieldMaskArg  syncMode,
                                  const UInt32             uiSyncInfo)
{
    Inherited::execSync(pFrom, whichField, oOffsets, syncMode, uiSyncInfo);

    if(FieldBits::NoField != (ShadowColorFieldMask & whichField))
        _sfShadowColor.syncWith(pFrom->_sfShadowColor);

    if(FieldBits::NoField != (ForceTextureUnitFieldMask & whichField))
        _sfForceTextureUnit.syncWith(pFrom->_sfForceTextureUnit);
}
#endif


inline
const Char8 *VRShadowEngineBase::getClassname(void)
{
    return "VRShadowEngine";
}
OSG_GEN_CONTAINERPTR(VRShadowEngine);


//#include "OSGVRShadowEngine.inl"

OSG_END_NAMESPACE

#endif // VRSHADOWENGINE_H_INCLUDED
