#ifndef VROBJECTFWD_H_INCLUDED
#define VROBJECTFWD_H_INCLUDED

#include "core/utils/VRFwdDeclTemplate.h"

namespace OSG {

// objects
ptrFwd(OSGCore);
ptrFwd(OSGObject);
ptrFwd(OSGTransform);
ptrFwd(OSGGeometry);
ptrFwd(OSGMaterial);
ptrFwd(OSGCamera);

ptrFwd(VRStorage);
ptrFwd(VRObject);
ptrFwd(VRTransform);
ptrFwd(VRGeometry);
ptrFwd(VRGeoData);
ptrFwd(VRCamera);
ptrFwd(VRLod);
ptrFwd(VRLodLeaf);
ptrFwd(VRSprite);
ptrFwd(VRStroke);
ptrFwd(VRMaterial);
ptrFwd(VRLight);
ptrFwd(VRLightBeacon);
ptrFwd(VRGroup);
ptrFwd(VRBillboard);
ptrFwd(VRStage);
ptrFwd(VRSky);
ptrFwd(VRRain); //temp

// other
ptrFwd(VRBackground);
ptrFwd(VRBackgroundBase);
ptrFwd(VRTexture);
ptrFwd(VRTextureGenerator);
ptrFwd(VRConstraint);
ptrFwd(VRAnimation);

// addons
ptrFwd(VRLodTree);
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
