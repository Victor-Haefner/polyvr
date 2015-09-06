#ifndef VRPROPERTY_H_INCLUDED
#define VRPROPERTY_H_INCLUDED

#include "VROntologyUtils.h"

using namespace std;

struct VRProperty : public VRNamedID {
    string value;
    string type;

    VRProperty(string name, string type);
};

#endif
