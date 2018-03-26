#ifndef VRSCENEFWD_H_INCLUDED
#define VRSCENEFWD_H_INCLUDED

#include <memory>

#define ptrFwd( X ) \
class X; \
typedef std::shared_ptr<X> X ## Ptr; \
typedef std::weak_ptr<X> X ## WeakPtr;

namespace OSG {

ptrFwd(VRSceneManager);
ptrFwd(VRProjectEntry);
ptrFwd(VRProjectsList);
ptrFwd(VRSemanticManager);
ptrFwd(VRPhysicsManager);
ptrFwd(VRRenderManager);
ptrFwd(VRRenderStudio);
ptrFwd(VRScene);
ptrFwd(VRThread);
ptrFwd(VRSpaceWarper);
ptrFwd(VRScenegraphInterface);

}

#endif // VRSCENEFWD_H_INCLUDED
