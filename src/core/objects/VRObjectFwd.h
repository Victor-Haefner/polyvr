#ifndef VROBJECTFWD_H_INCLUDED
#define VROBJECTFWD_H_INCLUDED

#include <memory>

#define ptrFwd( X ) \
class X; \
typedef std::shared_ptr<X> X ## Ptr; \
typedef std::weak_ptr<X> X ## WeakPtr;

namespace OSG {

// objects
ptrFwd(VRObject);
ptrFwd(VRTransform);
ptrFwd(VRGeometry);
ptrFwd(VRCamera);
ptrFwd(VRLod);
ptrFwd(VRSprite);
ptrFwd(VRStroke);
ptrFwd(VRMaterial);
ptrFwd(VRLight);
ptrFwd(VRLightBeacon);
ptrFwd(VRGroup);
ptrFwd(VRBillboard);
ptrFwd(VRStage);

// other
ptrFwd(VRTexture);

// tools
ptrFwd(VRAnnotationEngine);
ptrFwd(VRAnalyticGeometry);
ptrFwd(VRClipPlane);
ptrFwd(VRMenu);
ptrFwd(VRWaypoint);
ptrFwd(VRMeasure);
ptrFwd(VRTextureRenderer);

// addons
ptrFwd(VRMetaBalls);
ptrFwd(VRBlinds);
ptrFwd(VROpening);
ptrFwd(VRMolecule);
ptrFwd(VRTree);
ptrFwd(CSGGeometry);
ptrFwd(VRMillingWorkPiece);
ptrFwd(VRMillingCuttingToolProfile);

}

ptrFwd(VRNumberingEngine);
ptrFwd(VRLODSpace);

#endif // VROBJECTFWD_H_INCLUDED
