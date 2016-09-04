#ifndef VRSEMANTICSFWD_H_INCLUDED
#define VRSEMANTICSFWD_H_INCLUDED

#include <memory>

#define ptrFwd( X ) \
class X; \
typedef std::shared_ptr<X> X ## Ptr; \
typedef std::weak_ptr<X> X ## WeakPtr;

namespace OSG {

ptrFwd(VRConcept);
ptrFwd(VRProperty);
ptrFwd(VROntology);
ptrFwd(VREntity);
ptrFwd(VROntologyRule);
ptrFwd(VROntologyLink);
ptrFwd(VRReasoner);
ptrFwd(VRStatement);
ptrFwd(Variable);

}

#endif // VRSEMANTICSFWD_H_INCLUDED
