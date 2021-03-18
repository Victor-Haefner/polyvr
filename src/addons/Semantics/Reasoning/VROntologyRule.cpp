#include "VROntology.h"
#include "VRReasoner.h"
#include "VRProperty.h"
#include "core/utils/VRStorage_template.h"

#include <iostream>
#include <algorithm>

using namespace OSG;

VROntologyRule::VROntologyRule(string r, string ac) {
    associatedConcept = ac;
    rule = r;
    setRule(r);

    setStorageType("Rule");
    store("associatedConcept", &associatedConcept);
    storeObj("query", query);
    storeObjVec("statements", statements, true);

    auto ns = setNameSpace("VRRule");
    ns->setUniqueNames(false);
    setName("rule");
}

VROntologyRulePtr VROntologyRule::create(string rule, string ac) { return VROntologyRulePtr( new VROntologyRule(rule, ac) ); }

void VROntologyRule::setRule(string r) {
    statements.clear();
    vector<string> parts = VRReasoner::split(r, ':');
    if (parts.size() > 0) setQuery(parts[0]);
    if (parts.size() > 1) {
        parts = VRReasoner::split(parts[1], ';');
        for (unsigned int i=0; i<parts.size(); i++) statements.push_back(VRStatement::create(parts[i], i));
    }
}

void VROntologyRule::setQuery(string s) { query = VRStatement::create(s); }
//void VROntologyRule::setStatement(string s, int i) { statements[i]; }

void VROntologyRule::addAnnotation(VRPropertyPtr p) { annotations[p->ID] = p; }

string VROntologyRule::toString() {
    string res;
    if (query) res = query->toString();
    res += ":";
    for (auto s : statements) res += s->toString() + ";";
    res.pop_back();
    return res;
}

VRStatementPtr VROntologyRule::addStatement(string name) {
    auto s = VRStatement::create(name, statements.size());
    statements.push_back(s);
    return s;
}

VRStatementPtr VROntologyRule::getStatement(int i) {
    if (i < int(statements.size())) return statements[i];
    return 0;
}

void VROntologyRule::remStatement(VRStatementPtr s) {
    statements.erase( remove(statements.begin(), statements.end(), s), statements.end() );
}
