#include "VRBRepBound.h"
#include "core/utils/toString.h"
#include <OpenSG/OSGVector.h>

using namespace OSG;

VRBRepBound::VRBRepBound() {}

bool VRBRepBound::isClosed() {
    for (uint i=1; i<edges.size(); i++) {
        if (!edges[i-1].connectsTo(edges[i])) return false;
    }
    return true;
}

string VRBRepBound::edgeEndsToString() {
    string r;
    for (auto& e : edges) {
        r += " [" + toString(*e.EBeg) + " - " + toString(*e.EEnd) + "] ";
    }
    return r;
}
