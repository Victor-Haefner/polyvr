#include "VROntology.h"
#include "VRReasoner.h"

#include <iostream>

VROntologyRule::VROntologyRule(string rule) {
    this->name = "rule";
    this->rule = rule;
}
