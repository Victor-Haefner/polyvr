#ifndef VRSELECTIONFWD_H_INCLUDED
#define VRSELECTIONFWD_H_INCLUDED

#include <memory>

#define ptrFwd( X ) \
class X; \
typedef std::shared_ptr<X> X ## Ptr; \
typedef std::weak_ptr<X> X ## WeakPtr;

namespace OSG {

ptrFwd(VRSelector);
ptrFwd(VRSelection);
ptrFwd(VRPatchSelection);
ptrFwd(VRVRPolygonSelection);

}

#endif // VRSELECTIONFWD_H_INCLUDED
