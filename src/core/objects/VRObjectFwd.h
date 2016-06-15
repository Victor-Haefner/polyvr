#ifndef VROBJECTFWD_H_INCLUDED
#define VROBJECTFWD_H_INCLUDED

#include "core/utils/VRFwdDeclTemplate.h"

namespace OSG {

// objects
ptrFwd(OSGCore);
ptrFwd(OSGObject);
ptrFwd(OSGTransform);

ptrFwd(VRStorage);
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
ptrFwd(VRConstraint);

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
