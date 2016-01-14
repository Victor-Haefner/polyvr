#ifndef VRONTOLOGYRULE_H_INCLUDED
#define VRONTOLOGYRULE_H_INCLUDED

#include "VROntologyUtils.h"
#include "addons/Semantics/VRSemanticsFwd.h"

using namespace std;

struct VROntologyRule : public VRNamedID {
    string rule;
    VROntologyRule(string rule);

    static VROntologyRulePtr create(string rule);
    string toString();
};

#endif
