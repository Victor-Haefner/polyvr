#ifndef VRSCENEFWD_H_INCLUDED
#define VRSCENEFWD_H_INCLUDED

#include <memory>

#define ptrFwd( X ) \
class X; \
typedef std::shared_ptr<X> X ## Ptr; \
typedef std::weak_ptr<X> X ## WeakPtr;

namespace OSG {

ptrFwd(VRSceneManager);
ptrFwd(VRScene);

}

#endif // VRSCENEFWD_H_INCLUDED
