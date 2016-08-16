#include "VROntologyUtils.h"

int guid() {
    static int id = 0;
    id++;
    return id;
}

VROntoID::VROntoID() {
    ID = guid();
}
