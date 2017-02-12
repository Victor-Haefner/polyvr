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
ptrFwd(VRSemanticContext);
ptrFwd(VRReasoner);
ptrFwd(VRStatement);
ptrFwd(VRSemanticBuiltin);
ptrFwd(Variable);
ptrFwd(VRProcess);
ptrFwd(VRProcessNode);
ptrFwd(VRProcessLayout);
ptrFwd(VRProcessEngine);

}

#endif // VRSEMANTICSFWD_H_INCLUDED
