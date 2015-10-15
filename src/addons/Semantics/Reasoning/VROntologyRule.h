#ifndef VRONTOLOGYRULE_H_INCLUDED
#define VRONTOLOGYRULE_H_INCLUDED

#include "VROntologyUtils.h"

using namespace std;

struct VROntologyRule : public VRNamedID {
    string rule;
    VROntologyRule(string rule);
};

#endif
