#ifndef VRSELECTIONFWD_H_INCLUDED
#define VRSELECTIONFWD_H_INCLUDED

#include <memory>

#define ptrFwd( X ) \
class X; \
typedef std::shared_ptr<X> X ## Ptr; \
typedef std::weak_ptr<X> X ## WeakPtr;

namespace OSG {

ptrFwd(VRSelection);
ptrFwd(VRPatchSelection);
ptrFwd(VRPolygonSelection);

}

#endif // VRSELECTIONFWD_H_INCLUDED
