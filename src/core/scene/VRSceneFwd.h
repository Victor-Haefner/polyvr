#ifndef VRSCENEFWD_H_INCLUDED
#define VRSCENEFWD_H_INCLUDED

#include <memory>

#define ptrFwd( X ) \
class X; \
typedef std::shared_ptr<X> X ## Ptr; \
typedef std::weak_ptr<X> X ## WeakPtr;

namespace OSG {

ptrFwd(VRSceneManager);
ptrFwd(VRSemanticManager);
ptrFwd(VRPhysicsManager);
ptrFwd(VRScene);
ptrFwd(VRThread);

}

#endif // VRSCENEFWD_H_INCLUDED
