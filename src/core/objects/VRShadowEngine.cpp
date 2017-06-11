
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

#include <cstdlib>
#include <cstdio>

#include <OpenSG/OSGConfig.h>

#include "VRShadowEngine.h"

#include <OpenSG/OSGMatrixCamera.h>

#include <OpenSG/OSGMatrixUtility.h>
#include <OpenSG/OSGRenderAction.h>

#include <OpenSG/OSGRenderPartition.h>
#include <OpenSG/OSGFrameBufferObject.h>
#include <OpenSG/OSGGroup.h>
#include <OpenSG/OSGChunkMaterial.h>

#include <OpenSG/OSGTextureBuffer.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGPolygonChunk.h>
#include <OpenSG/OSGTexGenChunk.h>
#include <OpenSG/OSGBlendChunk.h>
#include <OpenSG/OSGImage.h>

OSG_USING_NAMESPACE

// Documentation for this class is emited in the
// OSGVRShadowEngineBase.cpp file.
// To modify it, please change the .fcd file (OSGVRShadowEngine.fcd) and
// regenerate the base file.


/*! \class OSG::VRShadowEngine
*/

/*-------------------------------------------------------------------------*/
/*                               Sync                                      */

void VRShadowEngine::changed(ConstFieldMaskArg whichField,
                                    UInt32            origin,
                                    BitVector         details)
{
    Inherited::changed(whichField, origin, details);
}

/*-------------------------------------------------------------------------*/
/*                               Dump                                      */

void VRShadowEngine::dump(      UInt32    uiIndent,
                                 const BitVector bvFlags) const
{
   Inherited::dump(uiIndent, bvFlags);
}

/*-------------------------------------------------------------------------*/
/*                            Constructors                                 */

VRShadowEngine::VRShadowEngine(void) :
    Inherited(    )
{
}

VRShadowEngine::VRShadowEngine(
    const VRShadowEngine &source) :

    Inherited(source)
{
}

/*-------------------------------------------------------------------------*/
/*                             Destructor                                  */

VRShadowEngine::~VRShadowEngine(void)
{
}


/*-------------------------------------------------------------------------*/
/*                                Init                                     */

void VRShadowEngine::initMethod(InitPhase ePhase)
{
    Inherited::initMethod(ePhase);

    if(ePhase == TypeObject::SystemPost)
    {
    }
}

void VRShadowEngine::lightRenderEnter(Light        *pLight,
                                             RenderAction *pAction)
{
    if(pLight->getOn() == false)
        return;

    StateChunk   *pChunk          = pLight->getChunk();

    UInt32        uiSlot          = pChunk->getClassId();

    Int32         iLightIndex     = pAction->allocateLightIndex();

//    LightChunk   *pLightChunk     = dynamic_cast<LightChunk *>(pChunk);

//    Color4f tmpVal(0.0, 0.0, 0.0, 1.0);

//    pLightChunk->setAmbient(tmpVal);


    if(iLightIndex >= 0)
    {
        pAction->addOverride(uiSlot + iLightIndex, pChunk);
    }
    else
    {
        SWARNING << "maximum light source limit ("
                 << -iLightIndex
                 << ") is reached"
                 << " skipping light sources!"
                 << std::endl;
    }
}

void VRShadowEngine::setupCamera(Light         *pLight,
                                        LightTypeE     eType,
                                        RenderAction  *pAction,
                                        EngineDataPtr  pEngineData)
{
    if(eType == Directional)
    {
        DirectionalLight *pDLight =
            dynamic_cast<DirectionalLight *>(pLight);

        MatrixCameraUnrecPtr pCam =
            dynamic_cast<MatrixCamera *>(pEngineData->getCamera());

        if(pCam == NULL)
        {
            pCam = MatrixCamera::createLocal();

            pEngineData->setCamera(pCam);
        }


        Vec3f   diff;
        Pnt3f   center;
        Matrix  transMatrix;
        Node   *pNode = pAction->getActNode();

//        tmpDir = DirectionalLightPtr::dcast(_lights[i]);

        diff = (pNode->getVolume().getMax() -
                pNode->getVolume().getMin());

        Real32 sceneWidth = diff.length() * 0.5f;
        // Not final values. May get tweaked in the future

        Real32 sceneHeight = diff.length() * 0.5f;

        pNode->getVolume().getCenter(center);

        Vec3f lightdir = pDLight->getDirection();

        if(pLight->getBeacon() != NULL)
        {
            Matrix m = pLight->getBeacon()->getToWorld();

            m.mult(lightdir, lightdir);
        }

        MatrixLookAt(transMatrix,
                     center + lightdir,
                     center,
                     Vec3f(0,1,0));

        transMatrix.invert();

        Matrix proMatrix;

        proMatrix.setIdentity();

        MatrixOrthogonal( proMatrix,
                         -sceneWidth,   sceneWidth, -sceneHeight,
                          sceneHeight, -sceneWidth,  sceneWidth);


        pCam->setProjectionMatrix(proMatrix  );
        pCam->setModelviewMatrix (transMatrix);
    }
    else if(eType == Point)
    {
        PointLight *pPLight = dynamic_cast<PointLight *>(pLight);

        MatrixCameraUnrecPtr pCam =
            dynamic_cast<MatrixCamera *>(pEngineData->getCamera());

        if(pCam == NULL)
        {
            pCam = MatrixCamera::createLocal();

            pEngineData->setCamera(pCam);
        }

        Real32  angle;
        Vec3f   dist;
        Pnt3f   center;
        Vec3f   diff;

        Matrix  transMatrix;

        Node   *pNode = pAction->getActNode();


        pNode->getVolume().getCenter(center);

        Pnt3f lightpos = pPLight->getPosition();

        if(pLight->getBeacon() != NULL)
        {
            Matrix m = pLight->getBeacon()->getToWorld();

            m.mult(lightpos, lightpos);
        }


        MatrixLookAt(transMatrix,
                     lightpos,
                     center,
                     Vec3f(0,1,0));

        transMatrix.invert();


        diff = (pNode->getVolume().getMax() -
                pNode->getVolume().getMin());

        dist  = lightpos - center;

        angle = atan((diff.length() * 0.5) / dist.length());

        Matrix proMatrix;

        proMatrix.setIdentity();

        MatrixPerspective( proMatrix,
                           2.f * angle,
                           1,
                           pAction->getActivePartition()->getNear(),
                           pAction->getActivePartition()->getFar ());


        pCam->setProjectionMatrix(proMatrix  );
        pCam->setModelviewMatrix (transMatrix);
    }
}

void VRShadowEngine::setupLightChunk(Light         *pLight,
                                            LightTypeE     eType,
                                            RenderAction  *pAction,
                                            EngineDataPtr  pEngineData)
{
    if(eType == Directional)
    {
        DirectionalLight *pDLight =
            dynamic_cast<DirectionalLight *>(pLight);

        LightChunkUnrecPtr  pChunk  = pEngineData->getLightChunk();

        if(pChunk == NULL)
        {
            pChunk = LightChunk::createLocal();

            pEngineData->setLightChunk(pChunk);
        }

        Color4f tmpVal(0.0, 0.0, 0.0, 1.0);

        pChunk->setSpecular(tmpVal);

        tmpVal.setValuesRGBA(0.2f, 0.2f, 0.2f, 1.0f);

        pChunk->setDiffuse (tmpVal);

        tmpVal.setValuesRGBA(0.0, 0.0, 0.0, 1.0);

        pChunk->setAmbient (tmpVal);

        Vec4f dir(pDLight->getDirection());

        dir[3] = 0;

        pChunk->setPosition(dir);

        pChunk->setBeacon(pLight->getBeacon());
    }
    else if(eType == Point)
    {
        PointLight         *pPLight = dynamic_cast<PointLight *>(pLight);

        LightChunkUnrecPtr  pChunk  = pEngineData->getLightChunk();

        if(pChunk == NULL)
        {
            pChunk = LightChunk::createLocal();

            pEngineData->setLightChunk(pChunk);
        }

        Color4f tmpVal(0.0, 0.0, 0.0, 1.0);

        pChunk->setSpecular(tmpVal);

        tmpVal.setValuesRGBA(this->getShadowColor()[0],
                             this->getShadowColor()[1],
                             this->getShadowColor()[2],
                             this->getShadowColor()[3]);

        pChunk->setDiffuse (tmpVal);

        tmpVal.setValuesRGBA(0.0, 0.0, 0.0, 1.0);

        pChunk->setAmbient (tmpVal);

        Vec4f pos(pPLight->getPosition());

        pos[3] = 1;

        pChunk->setPosition(pos);

        pChunk->setBeacon(pLight->getBeacon());
    }
}


void VRShadowEngine::doLightPass(Light         *pLight,
                                        RenderAction  *pAction,
                                        EngineDataPtr  pEngineData)
{
    this->pushPartition(pAction);

    RenderPartition   *pPart   = pAction    ->getActivePartition();
    Viewarea          *pArea   = pAction    ->getViewarea       ();
    Background        *pBack   = pAction    ->getBackground     ();

    FrameBufferObject *pTarget = pEngineData->getRenderTarget();

    if(pTarget == NULL)
    {
        FrameBufferObjectUnrecPtr pFBO = FrameBufferObject::createLocal();

        pFBO->setWidth (this->getWidth ());
        pFBO->setHeight(this->getHeight());

        pEngineData->setRenderTarget(pFBO);

        pTarget = pFBO;
    }

    TextureObjChunk       *pTexChunk  = pEngineData->getTexChunk();


    TextureBufferUnrecPtr  pTexBuffer = pEngineData->getTexBuffer();

    if(pTexBuffer == NULL)
    {
        pTexBuffer = TextureBuffer::createLocal();

        pEngineData->setTexBuffer     (pTexBuffer);

        pTexBuffer->setTexture        (pTexChunk );
        pTarget   ->setDepthAttachment(pTexBuffer);
    }

    PolygonChunkUnrecPtr pPoly = pEngineData->getPolyChunk();

    if(pPoly == NULL)
    {
        pPoly = PolygonChunk::createLocal();

        pPoly->setOffsetBias  (this->getOffsetBias  ());
        pPoly->setOffsetFactor(this->getOffsetFactor());
        pPoly->setOffsetFill  (true                   );

        pEngineData->setPolyChunk(pPoly);
    }

    pPart->setRenderTarget(pTarget);

    if(pArea != NULL)
    {
        Camera *pCam = pEngineData->getCamera();

        pPart->setWindow  (pAction->getWindow());

        pPart->calcViewportDimension(0.f, 0.f, 1.f, 1.f,
                                     pTarget->getWidth    (),
                                     pTarget->getHeight   ());

        Matrix m, t;

        // set the projection
        pCam->getProjection          (m,
                                      pPart->getViewportWidth (),
                                      pPart->getViewportHeight());

        pCam->getProjectionTranslation(t,
                                       pPart->getViewportWidth (),
                                       pPart->getViewportHeight());


        pPart->setupProjection(m, t);

        pCam->getViewing(m,
                         pPart->getViewportWidth (),
                         pPart->getViewportHeight());


        pPart->setupViewing(m);

        pPart->setNear     (pCam->getNear());
        pPart->setFar      (pCam->getFar ());

        pPart->calcFrustum ();

        pPart->setBackground(pBack);
    }

    Node *pActNode = pAction->getActNode();

    pAction->overrideMaterial(_pLightPassMat, pActNode);

    pAction->pushState();

    UInt32 uiPolySlot  = pPoly->getClassId();

    pAction->addOverride     (uiPolySlot,     pPoly);

//    lightRenderEnter(pLight, pAction);

    pAction->useNodeList(false);

    this->recurseFrom(pAction, pLight);

    pAction->popState();

    pAction->overrideMaterial(NULL, pActNode);

    this->popPartition(pAction);
}

void VRShadowEngine::doAmbientPass(Light         *pLight,
                                          RenderAction  *pAction,
                                          EngineDataPtr  pEngineData)
{
    this->pushPartition(pAction,
                        (RenderPartition::CopyViewing      |
                         RenderPartition::CopyProjection   |
                         RenderPartition::CopyWindow       |
                         RenderPartition::CopyViewportSize |
                         RenderPartition::CopyFrustum      |
                         RenderPartition::CopyNearFar      ));

    LightChunk   *pChunk      = pEngineData->getLightChunk();

    UInt32        uiSlot      = pChunk->getClassId();

    Int32         iLightIndex = pAction->allocateLightIndex();

    pAction->pushState();

    if(iLightIndex >= 0)
    {
        pAction->addOverride(uiSlot + iLightIndex, pChunk);
    }
    else
    {
        SWARNING << "maximum light source limit ("
                 << -iLightIndex
                 << ") is reached"
                 << " skipping light sources!"
                 << std::endl;
    }

    pAction->useNodeList(false);

    this->recurseFrom(pAction, pLight);

    pAction->popState();

    this->popPartition(pAction);
}

void VRShadowEngine::doFinalPass(Light         *pLight,
                                        RenderAction  *pAction,
                                        EngineDataPtr  pEngineData)
{
    this->pushPartition(pAction,
                        (RenderPartition::CopyViewing      |
                         RenderPartition::CopyProjection   |
                         RenderPartition::CopyWindow       |
                         RenderPartition::CopyViewportSize |
                         RenderPartition::CopyFrustum      |
                         RenderPartition::CopyNearFar      ));

    FrameBufferObject *pTarget = pEngineData->getRenderTarget();

    if(pTarget == NULL)
    {
        FrameBufferObjectUnrecPtr pFBO = FrameBufferObject::createLocal();

        pFBO->setWidth (this->getWidth ());
        pFBO->setHeight(this->getHeight());

        pEngineData->setRenderTarget(pFBO);

        pTarget = pFBO;
    }

    BlendChunkUnrecPtr pBlender      = pEngineData->getBlendChunk();

    if(pBlender == NULL)
    {
        pBlender = BlendChunk::createLocal();

        pBlender->setSrcFactor(GL_ONE);
        pBlender->setDestFactor(GL_ONE);
        pBlender->setAlphaFunc(GL_GEQUAL);
        pBlender->setAlphaValue(0.99f);

        pEngineData->setBlendChunk(pBlender);
    }


    Matrix4f projectionMatrix, viewMatrix, biasMatrix;

    biasMatrix.setIdentity();
    biasMatrix.setScale(0.5);
    biasMatrix.setTranslate(0.5,0.5,0.5);

    MatrixCamera *pCam = dynamic_cast<MatrixCamera *>(
        pEngineData->getCamera());

    pCam->getProjection(projectionMatrix,
                        this->getWidth (),
                        this->getHeight());

    pCam->getViewing   (viewMatrix,
                        this->getWidth (),
                        this->getHeight());

    Matrix textureMatrix = biasMatrix;
    textureMatrix.mult(projectionMatrix);
    textureMatrix.mult(viewMatrix);

    textureMatrix.transpose();
    Vec4f ps = textureMatrix[0];
    Vec4f pt = textureMatrix[1];
    Vec4f pr = textureMatrix[2];
    Vec4f pq = textureMatrix[3];

    TexGenChunkUnrecPtr pTexGen = pEngineData->getTexGenChunk();

    if(pTexGen == NULL)
    {
        pTexGen = TexGenChunk::createLocal();

        pEngineData->setTexGenChunk(pTexGen);

        pTexGen->setEyeModelViewMode(TexGenChunk::EyeModelViewCamera);

        pTexGen->setGenFuncS(GL_EYE_LINEAR);
        pTexGen->setGenFuncT(GL_EYE_LINEAR);
        pTexGen->setGenFuncR(GL_EYE_LINEAR);
        pTexGen->setGenFuncQ(GL_EYE_LINEAR);
    }

    pTexGen->setGenFuncSPlane(ps);
    pTexGen->setGenFuncTPlane(pt);
    pTexGen->setGenFuncRPlane(pr);
    pTexGen->setGenFuncQPlane(pq);

    TextureObjChunkUnrecPtr pTexChunk = pEngineData->getTexChunk();

    if(pTexChunk == NULL)
    {
        pTexChunk = TextureObjChunk::createLocal();

        pEngineData->setTexChunk(pTexChunk);

        ImageUnrecPtr pImage = Image::createLocal();

            // creates a image without allocating main memory.

        pImage->set(Image::OSG_L_PF,
                    pTarget->getWidth (),
                    pTarget->getHeight(),
                    1,
                    1,
                    1,
                    0,
                    NULL,
                    Image::OSG_UINT8_IMAGEDATA,
                    false);


        pTexChunk->setImage         (pImage);
        pTexChunk->setInternalFormat(GL_DEPTH_COMPONENT32);
        pTexChunk->setExternalFormat(GL_DEPTH_COMPONENT);
        pTexChunk->setMinFilter     (GL_LINEAR);
        pTexChunk->setMagFilter     (GL_LINEAR);
        pTexChunk->setWrapS         (GL_CLAMP_TO_BORDER);
        pTexChunk->setWrapT         (GL_CLAMP_TO_BORDER);
//        pTexChunk->setEnvMode       (GL_MODULATE);
        pTexChunk->setTarget        (GL_TEXTURE_2D);

        pTexChunk->setCompareMode(GL_COMPARE_R_TO_TEXTURE);
        pTexChunk->setCompareFunc(GL_LEQUAL);
        pTexChunk->setDepthMode  (GL_INTENSITY);
    }

    pAction->pushState();

    UInt32 uiBlendSlot  = pBlender ->getClassId();
    UInt32 uiTexSlot    = pTexChunk->getClassId();
    UInt32 uiTexGenSlot = pTexGen  ->getClassId();

    if(this->getForceTextureUnit() != -1)
    {
        uiTexSlot    += this->getForceTextureUnit();
        uiTexGenSlot += this->getForceTextureUnit();
    }
    else
    {
        uiTexSlot    += 3;
        uiTexGenSlot += 3;
    }

    pAction->addOverride(uiBlendSlot,  pBlender );
    pAction->addOverride(uiTexSlot,    pTexChunk);
    pAction->addOverride(uiTexGenSlot, pTexGen  );

    lightRenderEnter(pLight, pAction);

    pAction->useNodeList(false);

    this->recurseFrom(pAction, pLight);

    pAction->popState();

    this->popPartition(pAction);
}


Action::ResultE VRShadowEngine::runOnEnter(
    Light        *pLight,
    LightTypeE    eType,
    RenderAction *pAction)
{
    EngineDataUnrecPtr pEngineData =
        pAction->getData<SimpleShadowMapEngineData *>(_iDataSlotId);

    if(pEngineData == NULL)
    {
        pEngineData = EngineData::createLocal();

        this->setData(pEngineData, _iDataSlotId, pAction);
    }

    BitVector bvMask = pAction->getPassMask() & (bvLightPassMask   |
                                                 bvAmbientPassMask |
                                                 bvDiffusePassMask );

/*
    fprintf(stderr, "%p %llu %llu | %llu %llu %llu\n",
            this,
            bvMask,
            (bvLightPassMask   |
             bvAmbientPassMask |
             bvDiffusePassMask ),
            bvLightPassMask,
            bvAmbientPassMask,
            bvDiffusePassMask);
 */

    if(0x0000 != bvMask)
    {
        if(0x0000 != (bvMask & bvDiffusePassMask))
        {
            this->recurseFrom(pAction, pLight);

            setupCamera    (pLight, eType, pAction, pEngineData);
            doFinalPass    (pLight,  pAction, pEngineData);
        }

        if(0x0000 != (bvMask & bvAmbientPassMask))
        {
            this->recurseFrom(pAction, pLight);

            setupLightChunk(pLight, eType, pAction, pEngineData);
        }

        if(0x0000 != (bvMask & bvLightPassMask))
        {
            this->recurseFrom(pAction, pLight);

            doLightPass    (pLight,        pAction, pEngineData);
        }
    }
    else
    {
        setupCamera    (pLight, eType, pAction, pEngineData);

        setupLightChunk(pLight, eType, pAction, pEngineData);

        pAction->addPassMask(bvDiffusePassMask);
        doFinalPass    (pLight,        pAction, pEngineData);
        pAction->subPassMask(bvDiffusePassMask);

        pAction->addPassMask(bvAmbientPassMask);
        doAmbientPass  (pLight,        pAction, pEngineData);
        pAction->subPassMask(bvAmbientPassMask);

        pAction->addPassMask(bvLightPassMask);
        doLightPass    (pLight,        pAction, pEngineData);
        pAction->subPassMask(bvLightPassMask);
    }

    return Action::Skip;
}

Action::ResultE VRShadowEngine::runOnLeave(
    Light        *pLight,
    LightTypeE    eType,
    RenderAction *pAction)
{
    return Action::Continue;
}
