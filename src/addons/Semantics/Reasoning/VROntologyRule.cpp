#include "VROntology.h"
#include "VRReasoner.h"

#include <iostream>

using namespace OSG;

VROntologyRule::VROntologyRule(string rule) {
    setStorageType("Rule");
    setNameSpace("rule");
    setUniqueName(false);
    setName("rule");
    this->rule = rule;
}

VROntologyRulePtr VROntologyRule::create(string rule) { return VROntologyRulePtr( new VROntologyRule(rule) ); }

string VROntologyRule::toString() { return rule; }
