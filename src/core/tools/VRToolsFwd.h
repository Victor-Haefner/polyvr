#ifndef VRTOOLSFWD_H_INCLUDED
#define VRTOOLSFWD_H_INCLUDED

#include <memory>

#define ptrFwd( X ) \
class X; \
typedef std::shared_ptr<X> X ## Ptr; \
typedef std::weak_ptr<X> X ## WeakPtr;

namespace OSG {

// tools
ptrFwd(VRAnnotationEngine);
ptrFwd(VRAnalyticGeometry);
ptrFwd(VRClipPlane);
ptrFwd(VRMenu);
ptrFwd(VRWaypoint);
ptrFwd(VRMeasure);
ptrFwd(VRTextureRenderer);
ptrFwd(VRProjectManager);
ptrFwd(VRHandle);
ptrFwd(VRGeoPrimitive);
ptrFwd(VRJointTool);
ptrFwd(VRUndoManager);
ptrFwd(VRUndoAtom);
ptrFwd(VRUndo);
ptrFwd(CarDynamics);

}

#endif // VRTOOLSFWD_H_INCLUDED
