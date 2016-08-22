#include "VROntology.h"
#include "VRReasoner.h"

#include <iostream>
#include <algorithm>

using namespace OSG;

VROntologyRule::VROntologyRule(string r, string ac) {
    associatedConcept = ac;
    rule = r;

    vector<string> parts = VRReasoner::split(r, ':');
    if (parts.size() > 1) {
        query = VRStatement::New(parts[0]);
        parts = VRReasoner::split(parts[1], ';');
        for (int i=0; i<parts.size(); i++) statements.push_back(VRStatement::New(parts[i], i));
    }

    setStorageType("Rule");
    store("associatedConcept", &associatedConcept);

    setNameSpace("rule");
    setUniqueName(false);
    setName("rule");
}

VROntologyRulePtr VROntologyRule::create(string rule, string ac) { return VROntologyRulePtr( new VROntologyRule(rule, ac) ); }

string VROntologyRule::toString() { return rule; }

VRStatementPtr VROntologyRule::addStatement(string name) {
    auto s = VRStatement::New(name, statements.size());
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
