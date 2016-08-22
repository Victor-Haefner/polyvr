#include "VROntology.h"
#include "VRReasoner.h"

#include <iostream>
#include <algorithm>

using namespace OSG;

VROntologyRule::VROntologyRule(string r, string ac) {
    associatedConcept = ac;
    rule = r;

    setStorageType("Rule");
    setNameSpace("rule");
    setUniqueName(false);
    setName("rule");

    store("associatedConcept", &associatedConcept);
}

VROntologyRulePtr VROntologyRule::create(string rule, string ac) { return VROntologyRulePtr( new VROntologyRule(rule, ac) ); }

string VROntologyRule::toString() { return rule; }

VRStatementPtr VROntologyRule::addStatement(string name) {
    auto s = VRStatement::New(name);
    statements.push_back(s);
    return s;
}

VRStatementPtr VROntologyRule::getStatement(int i) {
    if (i < statements.size()) return statements[i];
    return 0;
}

void VROntologyRule::remStatement(VRStatementPtr s) {
    statements.erase( remove(statements.begin(), statements.end(), s), statements.end() );
}
