#include "VROntologyUtils.h"

int guid() {
    static int id = 0;
    id++;
    return id;
}

VRNamedID::VRNamedID() {
    ID = guid();
}
