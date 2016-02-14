#include "VRBRepBound.h"

using namespace OSG;

VRBRepBound::VRBRepBound() {}

bool VRBRepBound::isClosed() {
    for (int i=1; i<edges.size(); i++) {
        if (!edges[i-1].connectsTo(edges[i])) return false;
    }
    return true;
}
